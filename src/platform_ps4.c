#include "platform.h"
#include "input_ps4.h"
#include "log.h"
#include "scanout_copy.h"
#include "audio_queue.h"
#include "audio_resampler.h"
#include <orbis/libkernel.h>
#include <orbis/VideoOut.h>
#include <orbis/Pad.h>
#include <orbis/UserService.h>
#include <orbis/AudioOut.h>
#include <orbis/Sysmodule.h>
/* SDK leaves this prototype unspecified; preserve the real signed result ABI. */
extern int32_t nps64_module_is_loaded(enum OrbisSysModuleInternal) __asm__("sceSysmoduleIsLoadedInternal");
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>

#ifdef NPS64_GPU
extern void nps64_boot_trace(const char*);
extern void nps64_boot_result(const char*,int);
extern int sceKernelReserveVirtualRange(void**,size_t,int,size_t);
#define BOOT(s) nps64_boot_trace(s)
#define RESULT(s,r) nps64_boot_result(s,r)
#else
#define BOOT(s) ((void)0)
#define RESULT(s,r) ((void)0)
#endif
static int video=-1,audio=-1,active;
static int64_t frame=1;
static int64_t pending_flip;
static uint32_t *canvas;
#ifdef NPS64_GPU
#define SCANOUT_BUFFERS 2
#else
#define SCANOUT_BUFFERS 3
#endif
static void *memory,*buffers[SCANOUT_BUFFERS];
static off_t memory_offset;
static size_t memory_size;
static OrbisKernelEqueue flip_queue;
static bool queue_created, memory_allocated, buffers_registered;
static int handles[4]={-1,-1,-1,-1}, users[4]={-1,-1,-1,-1};
static uint16_t rumble_strength[4][2];
static bool rumble_connected[4];
static OrbisPadVibeParam rumble_sent[4];
/* Core callbacks can run on the emulation thread; only the polling thread
 * touches ScePad handles. Libretro strong=0 / weak=1. */
bool platform_rumble(unsigned port,unsigned effect,uint16_t strength) {
    if(port>=4 || effect>=2 || !__atomic_load_n(&rumble_connected[port],__ATOMIC_ACQUIRE)) return false;
    __atomic_store_n(&rumble_strength[port][effect],strength,__ATOMIC_RELEASE);
    return true;
}
static void rumble_stop(unsigned port) {
    for(unsigned e=0;e<2;e++) __atomic_store_n(&rumble_strength[port][e],0,__ATOMIC_RELEASE);
    OrbisPadVibeParam off={0};
    if(handles[port]>=0) scePadSetVibration(handles[port],&off);
    rumble_sent[port]=off;
}
void platform_rumble_clear(void) {for(unsigned i=0;i<4;i++) rumble_stop(i);}
static unsigned char font[128*8*16];
static Nps64AudioQueue audio_queue;
static int audio_stop,audio_failed,audio_priority_result,audio_output_error;
static unsigned audio_blocks;
static pthread_t audio_thread;
static bool audio_thread_started;
static Nps64AudioResampler audio_resampler;
static int primary_user=-1;
static uint64_t clock_anchor_ticks;
static double clock_anchor_seconds,clock_seconds_per_tick;
static uint64_t read_ticks(void) {
#ifdef NPS64_TEST_TSC
    extern uint64_t nps64_test_tsc(void);
    return nps64_test_tsc();
#else
    unsigned lo,hi;
    __asm__ volatile("lfence; rdtsc" : "=a"(lo),"=d"(hi) :: "memory");
    return ((uint64_t)hi<<32)|lo;
#endif
}

void nps64_cpu_worker_enter(unsigned lane) {
    OrbisPthread self=scePthreadSelf();
    int32_t before=-1,after=-1;
    uint64_t allowed=0;
    int query=scePthreadGetprio(self,&before),result=0;
    /* Give rendering precedence over background application work, while
       AudioOut (400) stays ahead. Preserve an already higher priority and
       keep the OS placement; extra RAM does not create extra CPU cores. */
    if(!query && before>450)result=scePthreadSetprio(self,450);
    scePthreadGetprio(self,&after);
    scePthreadGetaffinity(self,&allowed);
    nps64_log("[cpu-thread] lane=%u priority_before=%d target=450 query=%d result=%d priority_after=%d allowed_mask=0x%llx\n",
              lane,before,query,result,after,(unsigned long long)allowed);
}

static void *audio_worker(void *unused) {
    (void)unused;
    _Alignas(16) int16_t blocks[2][NPS64_AUDIO_BLOCK*2];
    unsigned current=0;
    int priority=scePthreadSetprio(scePthreadSelf(),400);
    __atomic_store_n(&audio_priority_result,priority,__ATOMIC_RELEASE);
    /* AudioOut paces on the previous buffer. Prepare the other buffer while
       playback continues; NULL is a drain, reserved for shutdown. */
    while(!__atomic_load_n(&audio_stop,__ATOMIC_ACQUIRE)) {
        nps64_audio_take(&audio_queue,blocks[current]);
        int result=sceAudioOutOutput(audio,blocks[current]);
        if(result<0) {
            __atomic_store_n(&audio_output_error,result,__ATOMIC_RELAXED);
            __atomic_store_n(&audio_failed,1,__ATOMIC_RELEASE);break;
        }
        __atomic_add_fetch(&audio_blocks,1,__ATOMIC_RELAXED);
        current^=1;
    }
    /* Drain while stack buffers are still alive. */
    sceAudioOutOutput(audio,NULL);
    return NULL;
}

static bool load_system_modules(void) {
    static const struct { const char *name; enum OrbisSysModuleInternal id; } modules[]={
        {"SystemService",ORBIS_SYSMODULE_INTERNAL_SYSTEM_SERVICE},
        {"UserService",ORBIS_SYSMODULE_INTERNAL_USER_SERVICE},
        {"VideoOut",ORBIS_SYSMODULE_INTERNAL_VIDEO_OUT},
        {"AudioOut",ORBIS_SYSMODULE_INTERNAL_AUDIOOUT},
        {"Pad",ORBIS_SYSMODULE_INTERNAL_PAD}
    };
    for(unsigned i=0;i<sizeof(modules)/sizeof(modules[0]);i++) {
        BOOT(modules[i].name);
        int32_t result=nps64_module_is_loaded(modules[i].id);
        if(result!=0) result=(int32_t)sceSysmoduleLoadModuleInternal(modules[i].id);
        RESULT(modules[i].name,result);
        nps64_log("[module] %s result=0x%08x\n",modules[i].name,(unsigned)result);
        if(result!=0) {
            BOOT("platform: required module unavailable; initialization aborted");
            return false; /* Never call an unresolved PRX import. */
        }
    }
    return true;
}
bool platform_init(void) {
    BOOT("platform: system module loading begin");
    if(!load_system_modules()) return false;
    BOOT("platform: system modules ready");
    /* Same invariant-TSC approach used by DolphinPS4. Anchor once and avoid
       a kernel clock call for each frontend/renderer profiling sample. */
    BOOT("platform: clock begin");
    uint64_t frequency=sceKernelGetTscFrequency();
    if(frequency>=1000000 && frequency<=10000000000ULL) {
        clock_anchor_seconds=(double)sceKernelGetProcessTime()/1000000.0;
        clock_anchor_ticks=read_ticks();clock_seconds_per_tick=1.0/frequency;
    }
    BOOT("platform: clock complete; canvas calloc begin");
    canvas=calloc(SCREEN_W*SCREEN_H,4);
    BOOT(canvas?"platform: canvas ready":"platform: canvas allocation FAILED");
    if(!canvas) goto fail;
    BOOT("platform: UserService initialize begin");
    int result=sceUserServiceInitialize(NULL); RESULT("UserService initialize",result);
    BOOT("platform: initial user begin");
    result=sceUserServiceGetInitialUser(&primary_user); RESULT("initial user",result);
    BOOT("platform: Pad init begin");
    result=scePadInit(); RESULT("Pad init",result); if(result<0) goto fail;
    BOOT("platform: VideoOut open begin");
    video=sceVideoOutOpen(ORBIS_VIDEO_USER_MAIN,ORBIS_VIDEO_OUT_BUS_MAIN,0,NULL);
    RESULT("VideoOut open",video);
    if(video<0) goto fail;
    BOOT("platform: flip queue begin");
    if(sceKernelCreateEqueue(&flip_queue,"NPS64 flip")<0) goto fail;
    queue_created=true;
    if(sceVideoOutAddFlipEvent(flip_queue,video,NULL)<0) goto fail;
    BOOT("platform: flip queue ready; direct allocation begin");
    memory_size=((SCREEN_W*SCREEN_H*4*SCANOUT_BUFFERS+0x1fffff)/0x200000)*0x200000;
    if(sceKernelAllocateDirectMemory(0,sceKernelGetDirectMemorySize(),memory_size,0x200000,3,&memory_offset)<0) goto fail;
    memory_allocated=true;
    BOOT("platform: direct allocation ready; scanout mapping begin");
    int map_flags=0;
#ifdef NPS64_GPU
    /* Video mappings must also stay clear of libkernel's pthread objects. */
    const uintptr_t hints[]={0x1400000000ull,0x1800000000ull,0x1c00000000ull};
    for(unsigned i=0;i<3;i++) {
        void *candidate=(void*)hints[i];
        result=sceKernelReserveVirtualRange(&candidate,memory_size,0,0x200000);
        RESULT("scanout virtual reservation",result);
        if(result==0 && (uintptr_t)candidate>=0x400000000ull) {
            memory=candidate; map_flags=0x10; break;
        }
    }
    if(!memory) goto fail;
#endif
    void *mapped=memory;
    result=sceKernelMapDirectMemory(&mapped,memory_size,0x33,map_flags,memory_offset,0x200000);
    RESULT("scanout map",result);
    if(result<0) {memory=NULL;goto fail;}
    memory=mapped;
    BOOT("platform: scanout clear begin");
    buffers[0]=memory; for(unsigned i=1;i<SCANOUT_BUFFERS;i++) buffers[i]=(char*)memory+(size_t)i*SCREEN_W*SCREEN_H*4;
    memset(memory,0,memory_size);
    BOOT("platform: scanout clear complete; buffer attributes begin");
    OrbisVideoOutBufferAttribute attr={0};
    sceVideoOutSetBufferAttribute(&attr,0x80000000,1,0,SCREEN_W,SCREEN_H,SCREEN_W);
    BOOT("platform: register buffers begin");
    result=sceVideoOutRegisterBuffers(video,0,buffers,SCANOUT_BUFFERS,&attr);
    RESULT("register buffers",result); if(result<0) goto fail;
    buffers_registered=true;
    sceVideoOutSetFlipRate(video,0);
    nps64_log("[scanout] buffers=%u max_pending=%u\n",SCANOUT_BUFFERS,SCANOUT_BUFFERS-1);
#ifndef FONT_PATH
#define FONT_PATH "/app0/font.bin"
#endif
    BOOT("platform: font open begin");
    FILE *f=fopen(FONT_PATH,"rb");
    if(!f) goto fail;
    bool font_ok=fread(font,1,sizeof(font),f)==sizeof(font); fclose(f);
    if(!font_ok) goto fail;
    BOOT("platform: font ready; AudioOut init begin");
    if(sceAudioOutInit()>=0) audio=sceAudioOutOpen(ORBIS_USER_SERVICE_USER_ID_SYSTEM,ORBIS_AUDIO_OUT_PORT_TYPE_MAIN,0,NPS64_AUDIO_BLOCK,48000,ORBIS_AUDIO_OUT_PARAM_FORMAT_S16_STEREO);
    RESULT("AudioOut open",audio);
    if(audio>=0) {
        BOOT("platform: audio thread create begin");
        if(pthread_create(&audio_thread,NULL,audio_worker,NULL)==0) audio_thread_started=true;
        else { sceAudioOutClose(audio);audio=-1; }
    }
    BOOT("platform: audio initialization complete");
    nps64_log("PS4 video=%d audio=%d\n",video,audio);
    return true;
fail:
    BOOT("platform: initialization failed; cleanup begin");
    platform_shutdown(); return false;
}
uint32_t *platform_framebuffer(void) { return canvas; }
static bool wait_flip(int64_t required) {
    if(required<=0) return true;
    double timeout=platform_time()+3;
    for(;;) {
        OrbisVideoOutFlipStatus status;
        if(sceVideoOutGetFlipStatus(video,&status)<0) return false;
        if(status.flipArg>=required) break;
        OrbisKernelEvent event; int count; unsigned int wait_us=100000;
        sceKernelWaitEqueue(flip_queue,&event,1,&count,&wait_us);
        if(platform_time()>timeout) { nps64_log("Timeout VideoOut\n"); return false; }
    }
    return true;
}
bool platform_present(void) {
    /* With three buffers, copy the next frame while one flip is pending.
       A buffer submitted at frame-N is reusable only after frame-N+1 is
       displayed. Bound the queue to N-1 and never overwrite scanout in use. */
    static double wait_seconds,copy_seconds;
    static unsigned samples;
    double started=platform_time();
    if(!wait_flip(frame-(SCANOUT_BUFFERS-1))) return false;
    double ready=platform_time();
    nps64_scanout_copy(buffers[active],canvas,SCREEN_W*SCREEN_H*4);
    double copied=platform_time();
    wait_seconds+=ready-started;copy_seconds+=copied-ready;samples++;
    if(sceVideoOutSubmitFlip(video,active,ORBIS_VIDEO_OUT_FLIP_VSYNC,frame)<0) return false;
    pending_flip=frame++;
    active=(active+1)%SCANOUT_BUFFERS;
    if(samples==60){
        nps64_log("[scanout] wait_ms=%.3f copy_ms=%.3f samples=%u\n",1000*wait_seconds/samples,1000*copy_seconds/samples,samples);
        wait_seconds=copy_seconds=0;samples=0;
    }
    return true;
}
void platform_poll(Input pads[4]) {
    OrbisUserServiceLoginUserIdList list;
    memset(&list,0xff,sizeof(list)); sceUserServiceGetLoginUserIdList(&list);
    int ordered[4]={-1,-1,-1,-1},next=1;
    for(int i=0;i<4;i++) if(list.userId[i]==primary_user) ordered[0]=primary_user;
    for(int i=0;i<4;i++) if(list.userId[i]>=0 && list.userId[i]!=primary_user && next<4) ordered[next++]=list.userId[i];
    for(int i=0;i<4;i++) {
        memset(&pads[i],0,sizeof(pads[i]));
        int uid=ordered[i];
        if(uid!=users[i]) { __atomic_store_n(&rumble_connected[i],false,__ATOMIC_RELEASE);rumble_stop(i);if(handles[i]>=0) scePadClose(handles[i]); handles[i]=-1; users[i]=uid; }
        if(uid<0) continue;
        if(handles[i]<0) handles[i]=scePadOpen(uid,0,0,NULL);
        if(handles[i]<0) continue;
        OrbisPadData p; memset(&p,0,sizeof(p));
        if(scePadReadState(handles[i],&p)<0 || !p.connected) {
            if(__atomic_exchange_n(&rumble_connected[i],false,__ATOMIC_ACQ_REL)) rumble_stop(i);
            continue;
        }
        __atomic_store_n(&rumble_connected[i],true,__ATOMIC_RELEASE);
        OrbisPadVibeParam vibe={
            (uint8_t)((__atomic_load_n(&rumble_strength[i][0],__ATOMIC_ACQUIRE)+128u)/257u),
            (uint8_t)((__atomic_load_n(&rumble_strength[i][1],__ATOMIC_ACQUIRE)+128u)/257u)};
        if(memcmp(&vibe,&rumble_sent[i],sizeof(vibe)) && scePadSetVibration(handles[i],&vibe)>=0) rumble_sent[i]=vibe;
        pads[i]=input_ps4_decode(&p);
    }
}
typedef struct { int16_t samples[256*2];unsigned count; } AudioBatch;
static void audio_emit(void *context,int16_t l,int16_t r) {
    AudioBatch *batch=context;
    batch->samples[batch->count*2]=l;batch->samples[batch->count*2+1]=r;
    if(++batch->count==256) {
        nps64_audio_push_batch(&audio_queue,batch->samples,batch->count);
        batch->count=0;
    }
}
void platform_audio(const int16_t *samples,size_t frames,double rate) {
    if(!audio_thread_started || rate<8000 || rate>192000)return;
    if(__atomic_load_n(&audio_failed,__ATOMIC_ACQUIRE)) {
        static int reported;
        if(!reported){reported=1;nps64_log("[audio] AudioOut failed result=0x%08x\n",(unsigned)__atomic_load_n(&audio_output_error,__ATOMIC_RELAXED));}
        return;
    }
    AudioBatch batch={.count=0};
    nps64_resample(&audio_resampler,samples,frames,rate,audio_emit,&batch);
    nps64_audio_push_batch(&audio_queue,batch.samples,batch.count);
    static double next_report,previous_report;
    static unsigned previous_blocks;
    double now=platform_time();
    if(now>=next_report) {
        next_report=now+2;
        unsigned blocks=__atomic_load_n(&audio_blocks,__ATOMIC_RELAXED);
        double output_hz=previous_report>0 && now>previous_report?(blocks-previous_blocks)*1024.0/(now-previous_report):0;
        previous_blocks=blocks;previous_report=now;
        unsigned rd=__atomic_load_n(&audio_queue.read,__ATOMIC_ACQUIRE);
        unsigned wr=__atomic_load_n(&audio_queue.write,__ATOMIC_ACQUIRE);
        nps64_log("[audio] queued_ms=%.1f underruns=%u dropped=%u block=1024 prefill=5760 resampler=sinc32 input_hz=%.3f output_hz=%.1f priority_result=%d protocol=pipelined\n",
                   (wr-rd)/48.0,__atomic_load_n(&audio_queue.underruns,__ATOMIC_RELAXED),
                   __atomic_load_n(&audio_queue.dropped,__ATOMIC_RELAXED),audio_resampler.rate,output_hz,__atomic_load_n(&audio_priority_result,__ATOMIC_ACQUIRE));
    }
}
void platform_audio_clear(void) {
    /* Consumer clears its own read index on the next output boundary. */
    nps64_audio_reset(&audio_queue);
    nps64_resampler_reset(&audio_resampler);
}
double platform_time(void) {
    if(clock_seconds_per_tick) {
        uint64_t ticks=read_ticks();
        if(ticks>=clock_anchor_ticks) return clock_anchor_seconds+(ticks-clock_anchor_ticks)*clock_seconds_per_tick;
    }
    return sceKernelGetProcessTime()/1000000.0;
}
void platform_sleep(double seconds) {
    if(seconds>0) sceKernelUsleep((uint32_t)(seconds>0.05?50000:seconds*1e6));
}
void platform_text(int x,int y,const char *text,uint32_t color,int scale) {
    uint32_t *fb=platform_framebuffer();
    for(;*text;text++,x+=8*scale) {
        unsigned c=(unsigned char)*text; if(c>127)c='?';
        for(int gy=0;gy<16;gy++) for(int gx=0;gx<8;gx++) {
            if(font[c*128+gy*8+gx]<80) continue;
            for(int dy=0;dy<scale;dy++) for(int dx=0;dx<scale;dx++) {
                int px=x+gx*scale+dx,py=y+gy*scale+dy;
                if(px>=0 && py>=0 && px<SCREEN_W && py<SCREEN_H) fb[py*SCREEN_W+px]=color;
            }
        }
    }
}
void platform_shutdown(void) {
    platform_rumble_clear();
    free(canvas);canvas=NULL;
    for(int i=0;i<4;i++) if(handles[i]>=0) scePadClose(handles[i]);
    if(audio_thread_started) {
        __atomic_store_n(&audio_stop,1,__ATOMIC_RELEASE); pthread_join(audio_thread,NULL);
        audio_thread_started=false;
    }
    if(audio>=0) { sceAudioOutOutput(audio,NULL); sceAudioOutClose(audio); }
    if(video>=0) { wait_flip(pending_flip);if(buffers_registered) sceVideoOutUnregisterBuffers(video,0); sceVideoOutClose(video); }
    if(memory) sceKernelMunmap(memory,memory_size);
    if(memory_allocated) sceKernelReleaseDirectMemory(memory_offset,memory_size);
    if(queue_created) sceKernelDeleteEqueue(flip_queue);
}
