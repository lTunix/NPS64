/* Dedicated file descriptor: PS4 musl sends fd 1/2 to debug output even
 * after freopen/dup2. Keep diagnostics independent of that slow channel. */
#include "log.h"
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>
#ifdef __ORBIS__
#include <orbis/libkernel.h>
#endif
static int fd=-1,lock,failed;
bool nps64_log_init(const char *path) {
#ifdef __ORBIS__
    fd=sceKernelOpen(path,O_WRONLY|O_CREAT|O_APPEND,0666);
#else
    fd=open(path,O_WRONLY|O_CREAT|O_APPEND,0666);
#endif
    failed=fd<0; return !failed;
}
bool nps64_log_ok(void) {return fd>=0 && !__atomic_load_n(&failed,__ATOMIC_RELAXED);}
void nps64_vlog(const char *format,va_list args) {
    char text[2048]; int length=vsnprintf(text,sizeof(text),format,args);
    if(length<=0 || fd<0)return;
    size_t remaining=(size_t)length<sizeof(text)?(size_t)length:sizeof(text)-1,offset=0;
    while(__atomic_exchange_n(&lock,1,__ATOMIC_ACQUIRE)) {}
    while(remaining) {
#ifdef __ORBIS__
        size_t written=sceKernelWrite(fd,text+offset,remaining);
#else
        ssize_t result=write(fd,text+offset,remaining);
        size_t written=result>0?(size_t)result:0;
#endif
        if(!written || written>remaining) {__atomic_store_n(&failed,1,__ATOMIC_RELAXED);break;}
        offset+=written;remaining-=written;
    }
    __atomic_store_n(&lock,0,__ATOMIC_RELEASE);
}
void nps64_log(const char *format,...) {
    va_list args;va_start(args,format);nps64_vlog(format,args);va_end(args);
}
void nps64_log_close(void) {
    if(fd<0)return;
#ifdef __ORBIS__
    sceKernelFsync(fd);sceKernelClose(fd);
#else
    fsync(fd);close(fd);
#endif
    fd=-1;
}
