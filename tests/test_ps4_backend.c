/* Exercise the actual PS4 backend on Linux with fake kernel/video services. */
#include "platform.h"
#include "log.h"
#include <orbis/libkernel.h>
#include <orbis/VideoOut.h>
#include <orbis/Pad.h>
#include <orbis/UserService.h>
#include <orbis/AudioOut.h>
#include <orbis/Sysmodule.h>
#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
static unsigned modules_loaded,module_load_calls,user_init_calls;
static unsigned module_bit(enum OrbisSysModuleInternal id) {
    switch(id) {
        case ORBIS_SYSMODULE_INTERNAL_SYSTEM_SERVICE:return 1;
        case ORBIS_SYSMODULE_INTERNAL_USER_SERVICE:return 2;
        case ORBIS_SYSMODULE_INTERNAL_VIDEO_OUT:return 4;
        case ORBIS_SYSMODULE_INTERNAL_AUDIOOUT:return 8;
        case ORBIS_SYSMODULE_INTERNAL_PAD:return 16;
        default:assert(0);return 0;
    }
}
int32_t test_module_is_loaded(enum OrbisSysModuleInternal id) __asm__("sceSysmoduleIsLoadedInternal");
int32_t test_module_is_loaded(enum OrbisSysModuleInternal id) {
    if(getenv("TEST_MODULES_PRELOADED"))modules_loaded=31;
    return (modules_loaded & module_bit(id))?0:-1;
}
uint32_t sceSysmoduleLoadModuleInternal(enum OrbisSysModuleInternal id) {
    module_load_calls++;
    if(getenv("TEST_MODULE_DENIED") && id==ORBIS_SYSMODULE_INTERNAL_USER_SERVICE)return 0x805a1000;
    modules_loaded|=module_bit(id);return 0;
}
#ifdef NPS64_GPU
#define TEST_SCANOUT_BUFFERS 2
#else
#define TEST_SCANOUT_BUFFERS 3
#endif
static void *scanout[TEST_SCANOUT_BUFFERS],*mapped;
static unsigned queued_buffers[2],queue_count;
static uint64_t queued_due[2],queued_hash[2],displayed_hash;
static int64_t queued_frames[2];
static int displayed_buffer=-1;
static uint64_t scanout_hash(unsigned b) {
    const uint32_t *p=scanout[b];uint64_t hash=1469598103934665603ULL;
    for(unsigned i=0;i<SCREEN_W*SCREEN_H;i++){hash^=p[i];hash*=1099511628211ULL;}
    return hash;
}
static void check_scanout_retained(void) {
    if(displayed_buffer>=0)assert(scanout_hash(displayed_buffer)==displayed_hash);
    for(unsigned i=0;i<queue_count;i++)assert(scanout_hash(queued_buffers[i])==queued_hash[i]);
}
#ifdef NPS64_GPU
static void *reserved;
void nps64_boot_trace(const char *s) {(void)s;}
void nps64_boot_result(const char *s,int r) {(void)s;(void)r;}
int sceKernelReserveVirtualRange(void **p,size_t n,int flags,size_t align) {
    assert(flags==0 && align==0x200000);
    if(getenv("TEST_RESERVE_DENIED"))return -1;
    assert(!posix_memalign(&reserved,align,n));*p=reserved;return 0;
}
#endif
static int64_t submitted,completed;
static uint64_t clock_us;
static uint64_t flip_due;
static unsigned process_clock_reads;
uint64_t nps64_test_tsc(void) {return 123456789+clock_us*2400;}
uint64_t sceKernelGetTscFrequency(void) {return getenv("TEST_TSC")?2400000000ULL:0;}
static uint32_t slept;
int32_t sceUserServiceInitialize(void *p) {assert(modules_loaded==31);user_init_calls++;return 0;}
int32_t sceUserServiceGetInitialUser(int32_t *u) {*u=1;return 0;}
int32_t sceUserServiceGetLoginUserIdList(OrbisUserServiceLoginUserIdList *l) {for(int i=0;i<4;i++)l->userId[i]=i?-1:1;return 0;}
int32_t scePadInit(void) {assert(modules_loaded & 16);return 0;}
int32_t scePadOpen(int32_t u,int32_t t,int32_t i,void *p) {return 1;}
int32_t scePadClose(int32_t h) {return 0;}
int32_t scePadReadState(int32_t h,OrbisPadData *p) {memset(p,0,sizeof(*p));p->connected=1;p->buttons=0x4000;p->leftStick.x=p->leftStick.y=p->rightStick.x=p->rightStick.y=128;return 0;}
int32_t sceVideoOutOpen(OrbisUserServiceUserId u,int32_t b,int32_t i,const void *p) {assert(modules_loaded & 4);return 1;}
int32_t sceVideoOutClose(int32_t v) {return 0;}
int32_t sceVideoOutAddFlipEvent(OrbisKernelEqueue q,int32_t v,void *p) {return 0;}
int32_t sceVideoOutSetFlipRate(int32_t h,int32_t r) {return 0;}
void sceVideoOutSetBufferAttribute(void *a,uint32_t p,uint32_t t,uint32_t r,uint32_t w,uint32_t h,uint32_t s) {}
int32_t sceVideoOutRegisterBuffers(int32_t h,int32_t start,void *const *b,int32_t n,const OrbisVideoOutBufferAttribute *a) {assert(n==TEST_SCANOUT_BUFFERS);for(int i=0;i<n;i++)scanout[i]=b[i];return 0;}
int32_t sceVideoOutUnregisterBuffers(int32_t h,int n) {return 0;}
static void retire_flips(void) {
    check_scanout_retained();
    while(queue_count && queued_due[0]<=clock_us){
        completed=queued_frames[0];displayed_buffer=(int)queued_buffers[0];displayed_hash=queued_hash[0];
        for(unsigned i=1;i<queue_count;i++){
            queued_buffers[i-1]=queued_buffers[i];queued_due[i-1]=queued_due[i];
            queued_frames[i-1]=queued_frames[i];queued_hash[i-1]=queued_hash[i];
        }
        queue_count--;
    }
}
int32_t sceVideoOutSubmitFlip(int32_t h,int32_t b,uint32_t mode,int64_t f) {
    retire_flips();assert(queue_count<TEST_SCANOUT_BUFFERS-1);
    assert(b>=0 && b<TEST_SCANOUT_BUFFERS && b!=displayed_buffer);
    for(unsigned i=0;i<queue_count;i++)assert(queued_buffers[i]!=(unsigned)b);
    submitted=f;
    uint64_t due=(clock_us/16667+1)*16667;
    if(queue_count && due<=queued_due[queue_count-1])due=queued_due[queue_count-1]+16667;
    queued_buffers[queue_count]=(unsigned)b;queued_due[queue_count]=due;
    queued_frames[queue_count]=f;queued_hash[queue_count]=scanout_hash(b);queue_count++;
    flip_due=queued_due[0];return 0;
}
int32_t sceVideoOutGetFlipStatus(int32_t h,OrbisVideoOutFlipStatus *s) {retire_flips();memset(s,0,sizeof(*s));s->flipArg=completed;return 0;}
int32_t sceKernelCreateEqueue(OrbisKernelEqueue *q,const char *n) {*q=1;return 0;}
int32_t sceKernelDeleteEqueue(OrbisKernelEqueue q) {return 0;}
int32_t sceKernelWaitEqueue(OrbisKernelEqueue q,OrbisKernelEvent *e,int32_t n,int32_t *c,OrbisKernelUseconds *t) {assert(queue_count);flip_due=queued_due[0];if(clock_us<flip_due)clock_us=flip_due;retire_flips();*c=1;return 0;}
size_t sceKernelGetDirectMemorySize(void) {return 32*1024*1024;}
int32_t sceKernelAllocateDirectMemory(off_t a,off_t b,size_t n,size_t align,int32_t type,off_t *o) {*o=0;return 0;}
int32_t sceKernelMapDirectMemory(void **p,size_t n,int32_t prot,int32_t flags,off_t off,size_t a) {
#ifdef NPS64_GPU
    assert(flags==0x10 && *p==reserved && (uintptr_t)*p>=0x400000000ull);
    memset(reserved,0xa5,n);mapped=*p;return 0;
#else
    *p=mapped=calloc(1,n);return *p?0:-1;
#endif
}
int32_t sceKernelMunmap(void *p,size_t n) {free(p);return 0;}
int32_t sceKernelReleaseDirectMemory(off_t p,size_t n) {return 0;}
uint64_t sceKernelGetProcessTime(void) {process_clock_reads++;return clock_us;}
int32_t sceKernelUsleep(uint32_t t) {slept=t;clock_us+=t;return 0;}
static const int16_t *audio_pending;
static int16_t audio_snapshot[1024*2];
static const void *audio_addresses[2];
static unsigned audio_submits,audio_drains,audio_nonzero;
static int audio_priority;
static int32_t cpu_priority=700;
static unsigned cpu_priority_requests;
OrbisPthread scePthreadSelf(void){return (OrbisPthread)(uintptr_t)1;}
int32_t scePthreadGetprio(OrbisPthread thread,int32_t *priority) {
    assert(thread==(OrbisPthread)(uintptr_t)1);
    if(getenv("TEST_CPU_QUERY_DENIED"))return -1;
    *priority=cpu_priority;return 0;
}
void scePthreadGetaffinity(OrbisPthread thread,uint64_t *mask) {
    assert(thread==(OrbisPthread)(uintptr_t)1);*mask=0x3f;
}
int32_t scePthreadSetprio(OrbisPthread thread,int priority){
    assert(thread==(OrbisPthread)(uintptr_t)1 && (priority==400 || priority==450));
    if(priority==450) {
        cpu_priority_requests++;
        if(getenv("TEST_CPU_PRIORITY_DENIED"))return -1;
        cpu_priority=450;return 0;
    }
    audio_priority=priority;return getenv("TEST_AUDIO_PRIORITY_DENIED")?-1:0;
}
int32_t sceAudioOutInit(void) {assert(modules_loaded & 8);return getenv("TEST_AUDIO")?0:-1;}
int32_t sceAudioOutOpen(int32_t u,enum OrbisAudioOutPort t,int32_t i,uint32_t n,uint32_t f,uint32_t p) {
    (void)u;(void)t;(void)i;assert(n==1024 && f==48000 && p==ORBIS_AUDIO_OUT_PARAM_FORMAT_S16_STEREO);return 7;
}
int32_t sceAudioOutOutput(int32_t h,const void *p) {
    assert(h==7);
    /* Retain the previous pointer until the next output finishes. */
    if(audio_pending)assert(!memcmp(audio_pending,audio_snapshot,sizeof(audio_snapshot)));
    if(!p){audio_drains++;audio_pending=NULL;return 0;}
    unsigned n=__atomic_load_n(&audio_submits,__ATOMIC_RELAXED);
    if(n<2)audio_addresses[n]=p;
    else assert(p==audio_addresses[n%2] && audio_addresses[0]!=audio_addresses[1]);
    if(audio_pending)usleep(21333);
    memcpy(audio_snapshot,p,sizeof(audio_snapshot));audio_pending=p;
    const int16_t *samples=p;
    for(unsigned j=0;j<1024;j++)assert((samples[j*2]==0 && samples[j*2+1]==0) ||
                                      (samples[j*2]==1000 && samples[j*2+1]==-2000));
    if(samples[0])audio_nonzero++;
    __atomic_add_fetch(&audio_submits,1,__ATOMIC_RELEASE);return 0;
}
int32_t sceAudioOutClose(int32_t h) {assert(h==7 && !audio_pending);return 0;}
int32_t sceKernelOpen(const char *p,int32_t f,OrbisKernelMode m) {return open(p,f,m);}
size_t sceKernelWrite(int32_t f,const void *p,size_t n) {return (size_t)write(f,p,n);}
int32_t sceKernelClose(int32_t f) {return close(f);}
int32_t sceKernelFsync(int32_t f) {return fsync(f);}
int main(void) {
    assert(nps64_log_init("build/ps4-backend-test.log"));
#ifdef NPS64_GPU
    if(getenv("TEST_RESERVE_DENIED")){assert(!platform_init());nps64_log_close();puts("PASS: scanout reservation failure safely aborts initialization");return 0;}
#endif
    if(getenv("TEST_MODULE_DENIED")){assert(!platform_init());assert(!user_init_calls && !mapped);nps64_log_close();puts("PASS: missing UserService module aborts before unresolved import");return 0;}
    assert(platform_init());
    assert(module_load_calls==(getenv("TEST_MODULES_PRELOADED")?0:5));
    if(getenv("TEST_CPU_HIGH_PRIORITY"))cpu_priority=300;
    nps64_cpu_worker_enter(0);
    if(getenv("TEST_CPU_HIGH_PRIORITY"))assert(cpu_priority==300 && !cpu_priority_requests);
    else if(getenv("TEST_CPU_QUERY_DENIED"))assert(cpu_priority==700 && !cpu_priority_requests);
    else if(getenv("TEST_CPU_PRIORITY_DENIED"))assert(cpu_priority==700 && cpu_priority_requests==1);
    else assert(cpu_priority==450 && cpu_priority_requests==1);
    uint32_t *canvas=platform_framebuffer();
    assert(canvas!=scanout[0] && canvas!=scanout[1]);
    if(getenv("TEST_FLIP_JITTER")) {
        /* Variable emulation cost: slow VI followed by a cheap VI, mean
           18.5 ms. Exercise actual buffer reuse and a 50 Hz frontend clock. */
        uint64_t deadline=clock_us;
        for(unsigned i=0;i<300;i++) {
            clock_us+=(i%2)?3000:34000;canvas[0]=i;
            assert(platform_present());
            deadline+=20000;
            if(deadline+20000<clock_us)deadline=clock_us;
            if(deadline>clock_us)clock_us=deadline;
        }
        if(TEST_SCANOUT_BUFFERS==3)assert(clock_us<=6020000);
        printf("FLIP_MODEL buffers=%u frames=300 elapsed_ms=%.3f fps=%.3f\n",
               TEST_SCANOUT_BUFFERS,clock_us/1000.0,300000000.0/clock_us);
        platform_shutdown();assert(completed==submitted);nps64_log_close();return 0;
    }
    canvas[0]=0xff123456;
    assert(((uint32_t*)scanout[0])[0]==0); /* drawing must not touch WC memory */
    platform_text(0,0,"X",0xffabcdef,2);
    assert(platform_present());
    assert(((uint32_t*)scanout[0])[0]==canvas[0] && submitted==1);
    assert(!memcmp(scanout[0],canvas,SCREEN_W*SCREEN_H*4));
    assert(completed==0 && clock_us==0); /* CPU may run while flip is pending */
    assert(platform_framebuffer()==canvas);
    canvas[0]=0xff654321; assert(platform_present());
    assert(((uint32_t*)scanout[1])[0]==canvas[0] && submitted==2);
    assert(!memcmp(scanout[1],canvas,SCREEN_W*SCREEN_H*4));
    if(TEST_SCANOUT_BUFFERS==3)assert(clock_us==0 && completed==0 && queue_count==2);
    else assert(platform_time()>0.016 && platform_time()<0.017 && completed==1);
    uint64_t began=clock_us;
    /* 10 ms of emulation overlaps the previous vblank, without overwriting
       a displayed/pending buffer or allowing an unbounded queue. */
    for(int i=0;i<60;i++) {clock_us+=10000;canvas[0]=(uint32_t)i;assert(platform_present());}
    assert(clock_us-began>58*16667 && clock_us-began<=60*16667);
    began=clock_us;
    for(int i=0;i<60;i++) {clock_us+=20000;canvas[0]=(uint32_t)(i+60);assert(platform_present());}
    assert(clock_us-began==60*20000); /* no rounding every 20 ms frame to 33 ms */
    platform_sleep(1.0/60);assert(slept==16666);
    unsigned reads=process_clock_reads;
    for(int i=0;i<1000;i++) assert(platform_time()>0);
    assert(getenv("TEST_TSC") ? process_clock_reads==reads : process_clock_reads==reads+1000);
    Input p[4];platform_poll(p);assert(p[0].buttons==BTN_CONFIRM && p[0].raw_buttons==0x4000);
    nps64_log("dedicated log %d\n",123);assert(nps64_log_ok());
    if(getenv("TEST_AUDIO")) {
        int16_t sound[4000*2];for(unsigned i=0;i<4000;i++){sound[i*2]=1000;sound[i*2+1]=-2000;}
        unsigned before=__atomic_load_n(&audio_submits,__ATOMIC_ACQUIRE);
        platform_audio(sound,4000,32040);
        for(unsigned i=0;i<200 && __atomic_load_n(&audio_submits,__ATOMIC_ACQUIRE)<before+16;i++)usleep(10000);
        assert(__atomic_load_n(&audio_submits,__ATOMIC_ACQUIRE)>=before+16);
    }
    platform_shutdown();nps64_log_close();
    if(getenv("TEST_AUDIO")){assert(audio_priority==400 && audio_nonzero>0 && audio_drains==2);puts("PASS AudioOut: two retained buffers, continuous submissions, priority and live-buffer shutdown drain");}
    assert(completed==submitted); /* drain before freeing scanout */
    FILE *f=fopen("build/ps4-backend-test.log","rb");assert(f);
    char text[1024]={0};fread(text,1,sizeof(text)-1,f);fclose(f);assert(strstr(text,"dedicated log 123"));
    puts("PASS: PS4 canvas, bounded pipelined flips, vblank overlap, safe shutdown, clock, Pad and log");
    return 0;
}
