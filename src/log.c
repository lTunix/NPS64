#include "log.h"
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#ifdef NPS64_LOG_ASYNC
#include <pthread.h>
#include <time.h>
#endif
#ifdef __ORBIS__
#include <orbis/libkernel.h>
#endif
static int fd=-1,lock,failed;
static bool write_all(const char *text,size_t length){
    size_t offset=0;
    while(offset<length){
#ifdef __ORBIS__
        size_t written=sceKernelWrite(fd,text+offset,length-offset);
#else
        ssize_t result=write(fd,text+offset,length-offset);
        size_t written=result>0?(size_t)result:0;
#endif
        if(!written || written>length-offset){__atomic_store_n(&failed,1,__ATOMIC_RELAXED);return false;}
        offset+=written;
    }
    return true;
}
#ifdef NPS64_LOG_ASYNC
/* Bounded MPSC sequence queue. A producer reserves its own slot; filesystem
 * I/O never owns a producer lock. Only the consumer advances read_position.
 * Full queues drop diagnostics and count them instead of blocking gameplay. */
enum { LOG_CAPACITY=128,LOG_TEXT=2048,LOG_BATCH=16384 };
typedef struct {uint64_t sequence;size_t length;char text[LOG_TEXT];} LogSlot;
static LogSlot slots[LOG_CAPACITY];
static uint64_t write_position,read_position;
static unsigned dropped,producers;
static int accepting,stopping,asynchronous;
static pthread_t writer;
static bool enqueue(const char *text,size_t length){
    uint64_t position=__atomic_load_n(&write_position,__ATOMIC_RELAXED);
    for(;;){
        LogSlot *slot=&slots[position%LOG_CAPACITY];
        uint64_t sequence=__atomic_load_n(&slot->sequence,__ATOMIC_ACQUIRE);
        if(sequence==position){
            if(__atomic_compare_exchange_n(&write_position,&position,position+1,0,__ATOMIC_RELAXED,__ATOMIC_RELAXED)){
                memcpy(slot->text,text,length);slot->length=length;
                __atomic_store_n(&slot->sequence,position+1,__ATOMIC_RELEASE);return true;
            }
        }else if(position-sequence<UINT64_MAX/2){
            __atomic_add_fetch(&dropped,1,__ATOMIC_RELAXED);return false;
        }else position=__atomic_load_n(&write_position,__ATOMIC_RELAXED);
    }
}
static void idle_writer(void){
#ifdef __ORBIS__
    sceKernelUsleep(1000);
#else
    struct timespec delay={0,1000000};nanosleep(&delay,NULL);
#endif
}
static void *write_worker(void *unused){
    (void)unused;
#ifdef __ORBIS__
    scePthreadSetprio(scePthreadSelf(),750);
#endif
    char batch[LOG_BATCH];unsigned reported_drops=0;
    for(;;){
        size_t length=0;
        while(length+LOG_TEXT<=sizeof(batch)){
            LogSlot *slot=&slots[read_position%LOG_CAPACITY];
            if(__atomic_load_n(&slot->sequence,__ATOMIC_ACQUIRE)!=read_position+1)break;
            memcpy(batch+length,slot->text,slot->length);length+=slot->length;
            __atomic_store_n(&slot->sequence,read_position+LOG_CAPACITY,__ATOMIC_RELEASE);
            read_position++;
        }
        if(length && !__atomic_load_n(&failed,__ATOMIC_RELAXED))write_all(batch,length);
        unsigned total=__atomic_load_n(&dropped,__ATOMIC_RELAXED);
        if(total!=reported_drops){
            char notice[96];int n=snprintf(notice,sizeof(notice),"[log-queue] dropped=%u capacity=%u\n",total,LOG_CAPACITY);
            if(n>0 && !__atomic_load_n(&failed,__ATOMIC_RELAXED))write_all(notice,(size_t)n);
            reported_drops=total;
        }
        if(__atomic_load_n(&stopping,__ATOMIC_SEQ_CST) && !__atomic_load_n(&producers,__ATOMIC_SEQ_CST) && read_position==__atomic_load_n(&write_position,__ATOMIC_ACQUIRE))break;
        if(!length)idle_writer();
    }
    return NULL;
}
#endif
bool nps64_log_async(void){
#ifdef NPS64_LOG_ASYNC
    return asynchronous!=0;
#else
    return false;
#endif
}
unsigned nps64_log_dropped(void){
#ifdef NPS64_LOG_ASYNC
    return __atomic_load_n(&dropped,__ATOMIC_RELAXED);
#else
    return 0;
#endif
}
bool nps64_log_init(const char *path) {
    if(fd>=0)nps64_log_close();
#ifdef __ORBIS__
    fd=sceKernelOpen(path,O_WRONLY|O_CREAT|O_APPEND,0666);
#else
    fd=open(path,O_WRONLY|O_CREAT|O_APPEND,0666);
#endif
    failed=fd<0;
#ifdef NPS64_LOG_ASYNC
    asynchronous=0;write_position=read_position=0;dropped=producers=0;accepting=stopping=0;
    for(unsigned i=0;i<LOG_CAPACITY;i++)slots[i].sequence=i;
    if(fd>=0){
        __atomic_store_n(&accepting,1,__ATOMIC_SEQ_CST);
        if(pthread_create(&writer,NULL,write_worker,NULL)==0)asynchronous=1;
        else __atomic_store_n(&accepting,0,__ATOMIC_SEQ_CST);
    }
#endif
    return fd>=0;
}
bool nps64_log_ok(void) {return fd>=0 && !__atomic_load_n(&failed,__ATOMIC_RELAXED);}
void nps64_vlog(const char *format,va_list args) {
    char text[2048];int length=vsnprintf(text,sizeof(text),format,args);
    if(length<=0 || fd<0)return;
    size_t count=(size_t)length<sizeof(text)?(size_t)length:sizeof(text)-1;
#ifdef NPS64_LOG_ASYNC
    if(asynchronous){
        __atomic_add_fetch(&producers,1,__ATOMIC_SEQ_CST);
        if(__atomic_load_n(&accepting,__ATOMIC_SEQ_CST))enqueue(text,count);
        __atomic_sub_fetch(&producers,1,__ATOMIC_SEQ_CST);return;
    }
#endif
    while(__atomic_exchange_n(&lock,1,__ATOMIC_ACQUIRE)) {}
    write_all(text,count);
    __atomic_store_n(&lock,0,__ATOMIC_RELEASE);
}
void nps64_log(const char *format,...) {
    va_list args;va_start(args,format);nps64_vlog(format,args);va_end(args);
}
void nps64_log_close(void) {
    if(fd<0)return;
#ifdef NPS64_LOG_ASYNC
    if(asynchronous){
        __atomic_store_n(&accepting,0,__ATOMIC_SEQ_CST);
        __atomic_store_n(&stopping,1,__ATOMIC_SEQ_CST);
        pthread_join(writer,NULL);
    }
#endif
#ifdef __ORBIS__
    sceKernelFsync(fd);sceKernelClose(fd);
#else
    fsync(fd);close(fd);
#endif
    fd=-1;
#ifdef NPS64_LOG_ASYNC
    asynchronous=0;
#endif
}
