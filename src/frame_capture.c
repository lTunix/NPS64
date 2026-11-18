#include "frame_capture.h"
#include "log.h"
#include "platform.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>

/* Only the render thread arms/copies/finishes. The writer owns the immutable
 * batch after state=WRITING; no disk IO, GPU commands or extra VI in the burst. */
enum { IDLE,CAPTURING,WRITING,COUNT=16 };
typedef struct {
    uint64_t core,video,before,after;
    unsigned source_width,source_height,scale;
} Frame;
static struct {
    unsigned state,width,height,count;
    bool pending,started,stop;
    int result;
    size_t frame_bytes;
    uint32_t *pixels;
    Frame frames[COUNT];
    char directory[1024],rom[64],trigger[32];
    pthread_t thread;
} c;
static pthread_mutex_t mutex=PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t condition=PTHREAD_COND_INITIALIZER;
static uint64_t hash_pixels(const uint32_t *pixels,size_t count) {
    uint64_t hash=UINT64_C(1469598103934665603);
    for(size_t i=0;i<count;i++){hash^=pixels[i];hash*=UINT64_C(1099511628211);}
    return hash;
}
static bool write_ppm(const char *path,const uint32_t *pixels) {
    FILE *file=fopen(path,"wb");if(!file)return false;
    bool ok=fprintf(file,"P6\n%u %u\n255\n",c.width,c.height)>0;
    unsigned char row[1280*3];
    for(unsigned y=0;ok&&y<c.height;y++) {
        for(unsigned x=0;x<c.width;x++) {
            uint32_t p=pixels[(size_t)y*c.width+x];
            row[3*x]=(unsigned char)(p>>16);row[3*x+1]=(unsigned char)(p>>8);row[3*x+2]=(unsigned char)p;
        }
        ok=fwrite(row,3,c.width,file)==c.width;
    }
    if(fclose(file))ok=false;
    return ok;
}
static bool write_batch(void) {
    char path[1200];bool ok=true;
    snprintf(path,sizeof(path),"%s/frames.csv",c.directory);
    FILE *meta=fopen(path,"w");if(!meta)return false;
    if(fprintf(meta,"# rom=%s trigger=%s diagnostic_copies=1 performance_sample=0\nindex,core_frame,video_frame,source_width,source_height,scale,before_hash,after_hash\n",c.rom,c.trigger)<0)ok=false;
    for(unsigned i=0;i<c.count;i++) {
        Frame *f=&c.frames[i];
        f->before=hash_pixels(c.pixels+(size_t)i*2*(c.frame_bytes/4),c.frame_bytes/4);
        f->after=hash_pixels(c.pixels+((size_t)i*2+1)*(c.frame_bytes/4),c.frame_bytes/4);
        if(fprintf(meta,"%u,%llu,%llu,%u,%u,%u,%016llx,%016llx\n",i,(unsigned long long)f->core,(unsigned long long)f->video,f->source_width,f->source_height,f->scale,(unsigned long long)f->before,(unsigned long long)f->after)<0)ok=false;
        for(unsigned phase=0;phase<2;phase++) {
            snprintf(path,sizeof(path),"%s/%02u-%s.ppm",c.directory,i,phase?"scanout":"game");
            const uint32_t *pixels=c.pixels+((size_t)i*2+phase)*(c.frame_bytes/4);
            if(!write_ppm(path,pixels))ok=false;
        }
    }
    if(fclose(meta))ok=false;
    return ok;
}
static void *writer(void *unused) {
    (void)unused;
    for(;;) {
        pthread_mutex_lock(&mutex);
        while(__atomic_load_n(&c.state,__ATOMIC_ACQUIRE)!=WRITING && !c.stop)pthread_cond_wait(&condition,&mutex);
        bool ready=__atomic_load_n(&c.state,__ATOMIC_ACQUIRE)==WRITING;
        bool stop=c.stop;pthread_mutex_unlock(&mutex);
        if(ready) {
            bool ok=write_batch();
            nps64_log("[frame-capture] saved=%u frames=%u directory=%s diagnostic_only=1\n",ok,c.count,c.directory);
            free(c.pixels);c.pixels=NULL;
            __atomic_store_n(&c.result,ok?1:-1,__ATOMIC_RELEASE);
            __atomic_store_n(&c.state,IDLE,__ATOMIC_RELEASE);
        }
        if(stop)break;
    }
    return NULL;
}
bool nps64_frame_capture_arm(const char *root,const char *rom,unsigned width,unsigned height,const char *trigger) {
    if(!root||!rom||!trigger||!width||!height||width>1280||height>720||__atomic_load_n(&c.state,__ATOMIC_ACQUIRE)!=IDLE)return false;
    size_t bytes=(size_t)width*height*4;
    uint32_t *pixels=malloc(bytes*COUNT*2);if(!pixels)return false;
    if(mkdir(root,0777)&&errno!=EEXIST){free(pixels);return false;}
    bool made=false;
    for(unsigned i=0;i<100;i++) {
        int n=snprintf(c.directory,sizeof(c.directory),"%s/capture-%llu-%u",root,(unsigned long long)(platform_time()*1000000),i);
        if(n<0 || (size_t)n>=sizeof(c.directory))break;
        if(!mkdir(c.directory,0777)){made=true;break;}
        if(errno!=EEXIST)break;
    }
    if(!made){free(pixels);return false;}
    if(!c.started) {
        pthread_attr_t attr;
        if(pthread_attr_init(&attr)){free(pixels);return false;}
        int error=pthread_attr_setstacksize(&attr,256*1024);
        c.stop=false;
        if(!error)error=pthread_create(&c.thread,&attr,writer,NULL);
        pthread_attr_destroy(&attr);
        if(error){free(pixels);return false;}
        c.started=true;
    }
    c.pixels=pixels;c.frame_bytes=bytes;c.width=width;c.height=height;c.count=0;c.pending=false;
    snprintf(c.rom,sizeof(c.rom),"%s",rom);snprintf(c.trigger,sizeof(c.trigger),"%s",trigger);
    __atomic_store_n(&c.result,0,__ATOMIC_RELAXED);
    __atomic_store_n(&c.state,CAPTURING,__ATOMIC_RELEASE);
    nps64_log("[frame-capture] armed frames=%u bytes=%zu directory=%s trigger=%s\n",COUNT,bytes*COUNT*2,c.directory,c.trigger);
    return true;
}
void nps64_frame_capture_before(const uint32_t *pixels,uint64_t core,uint64_t video,unsigned width,unsigned height,unsigned scale) {
    if(!pixels || __atomic_load_n(&c.state,__ATOMIC_ACQUIRE)!=CAPTURING || c.pending)return;
    uint32_t *dst=c.pixels+(size_t)c.count*2*(c.frame_bytes/4);
    memcpy(dst,pixels,c.frame_bytes);
    Frame *f=&c.frames[c.count];*f=(Frame){.core=core,.video=video,.source_width=width,.source_height=height,.scale=scale};
    c.pending=true;
}
void nps64_frame_capture_after(const uint32_t *pixels) {
    if(!pixels || __atomic_load_n(&c.state,__ATOMIC_ACQUIRE)!=CAPTURING || !c.pending)return;
    uint32_t *dst=c.pixels+((size_t)c.count*2+1)*(c.frame_bytes/4);
    memcpy(dst,pixels,c.frame_bytes);
    c.pending=false;
    if(++c.count==COUNT)nps64_frame_capture_finish();
}
void nps64_frame_capture_finish(void) {
    if(__atomic_load_n(&c.state,__ATOMIC_ACQUIRE)!=CAPTURING)return;
    c.pending=false;
    if(!c.count){free(c.pixels);c.pixels=NULL;__atomic_store_n(&c.state,IDLE,__ATOMIC_RELEASE);return;}
    pthread_mutex_lock(&mutex);
    __atomic_store_n(&c.state,WRITING,__ATOMIC_RELEASE);pthread_cond_signal(&condition);
    pthread_mutex_unlock(&mutex);
}
void nps64_frame_capture_shutdown(void) {
    if(!c.started)return;
    nps64_frame_capture_finish();
    pthread_mutex_lock(&mutex);c.stop=true;pthread_cond_signal(&condition);pthread_mutex_unlock(&mutex);
    pthread_join(c.thread,NULL);c.started=false;
}
int nps64_frame_capture_result(void){return __atomic_exchange_n(&c.result,0,__ATOMIC_ACQ_REL);}
