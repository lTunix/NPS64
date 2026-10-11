/* Allocation-free startup trace and deferred GPU C++ initialization. */
#include <stdint.h>
#include <stddef.h>
#include <fcntl.h>
#include <orbis/libkernel.h>
void nps64_boot_trace(const char *text) {
    sceKernelMkdir("/data/NPS64",0777);
    int fd=sceKernelOpen("/data/NPS64/boot.log",O_WRONLY|O_CREAT|O_APPEND,0666);
    if(fd<0)return;
    size_t n=0;while(text[n])n++;
    sceKernelWrite(fd,"[boot] ",7);sceKernelWrite(fd,text,n);sceKernelWrite(fd,"\n",1);
    sceKernelFsync(fd);sceKernelClose(fd);
}
/* Format syscall results without stdio or heap allocation. */
void nps64_boot_result(const char *label,int result) {
    char line[160]; size_t n=0;
    while(*label && n<sizeof(line)-14) line[n++]=*label++;
    line[n++]=' ';line[n++]='0';line[n++]='x';
    for(int shift=28;shift>=0;shift-=4) line[n++]="0123456789abcdef"[((uint32_t)result>>shift)&15];
    line[n]=0;nps64_boot_trace(line);
}
static void early_boot(void) {
#if defined(NPS64_PARALLEL_TILE_PROBE)
    nps64_boot_trace("00.58 original tile capture before main; GPU constructors deferred");
#elif defined(NPS64_PARALLEL_GAME)
    nps64_boot_trace("00.53 original ParaLLEl-RDP before main; GPU constructors deferred");
#elif defined(NPS64_PARALLEL_PROBE)
    nps64_boot_trace("00.52 ParaLLEl compute gate before main; GPU constructors deferred");
#elif defined(NPS64_HLE_GAME)
    nps64_boot_trace("00.27 game geometry snapshot before main; GPU constructors deferred");
#elif defined(NPS64_HLE_PROBE)
    nps64_boot_trace("00.23 graphics probe before main; GPU constructors deferred");
#elif defined(NPS64_GPU_EXPERIMENTAL)
    nps64_boot_trace("00.19 experimental before main; GPU constructors deferred");
#else
    nps64_boot_trace("00.13 before main; GPU constructors deferred");
#endif
}
__attribute__((used,section(".nps64_boot_init")))
static void (*const boot_entry)(void)=early_boot;
extern void (*__nps64_gpu_init_start[])(void);
extern void (*__nps64_gpu_init_end[])(void);
void nps64_gpu_static_init(void) {
    static int initialized;
    if(initialized)return;
    for(void (**p)(void)=__nps64_gpu_init_start;p<__nps64_gpu_init_end;p++) {
        nps64_boot_trace("GPU constructor begin");
        (*p)();
        nps64_boot_trace("GPU constructor complete");
    }
    initialized=1;
}
void ps4_heap_error(void *heap,void *chunk,int corruption) {
    (void)heap;(void)chunk;
    nps64_boot_trace(corruption?"FATAL heap corruption":"FATAL invalid heap pointer");
    __builtin_trap();
}
