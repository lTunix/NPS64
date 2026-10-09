#include <assert.h>
#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#define MB ((size_t)1024*1024)
void *nps_malloc(size_t),*nps_realloc(void*,size_t),*nps_calloc(size_t,size_t);
void nps_free(void*);
int nps_posix_memalign(void**,size_t,size_t);
static int denied,release_count,map_count;
static void *reservation;
size_t sceKernelGetDirectMemorySize(void){return 4096*MB;}
int sceKernelAvailableDirectMemorySize(int64_t a,int64_t b,size_t c,int64_t *phys,size_t *size){(void)a;(void)b;(void)c;*phys=0;*size=2624*MB;return 0;}
int sceKernelAllocateDirectMemory(int64_t a,int64_t b,size_t n,size_t align,int type,int64_t *phys){(void)a;(void)b;assert(n==64*MB && align==2*MB && type==0);*phys=0;return 0;}
int sceKernelReserveVirtualRange(void **addr,size_t n,int flags,size_t align){assert(flags==0 && align==2*MB);if(denied)return -1;assert(!reservation);assert(!posix_memalign(&reservation,align,n));*addr=reservation;return 0;}
int sceKernelMapDirectMemory(void **addr,size_t n,int prot,int flags,int64_t phys,size_t align){assert(*addr==(char*)reservation+2*MB);assert((uintptr_t)*addr>=0x400000000ull && (uintptr_t)*addr%(2*MB)==0);assert(n==64*MB && prot==3 && flags==0x10 && phys==0 && align==2*MB);map_count++;return 0;}
int sceKernelReleaseDirectMemory(int64_t a,size_t n){(void)a;(void)n;release_count++;return 0;}
int sceKernelAvailableFlexibleMemorySize(size_t *n){*n=0;return 0;}
int sceKernelMapNamedFlexibleMemory(void **a,size_t n,int p,int f,const char *name){(void)a;(void)n;(void)p;(void)f;(void)name;assert(0);return -1;}
void nps64_boot_trace(const char *s){(void)s;}
void ps4_heap_error(void *a,void *b,int c){(void)a;(void)b;(void)c;assert(0);}
int main(void){
    pid_t child=fork();assert(child>=0);
    if(!child){denied=1;assert(!nps_malloc(16) && errno==ENOMEM && release_count==1);_exit(0);}
    int status;assert(waitpid(child,&status,0)==child && WIFEXITED(status) && WEXITSTATUS(status)==0);
    void *blocks[16384];
    for(unsigned i=0;i<16384;i++){size_t n=1+i%1536;blocks[i]=nps_malloc(n);assert(blocks[i] && (uintptr_t)blocks[i]%16==0);memset(blocks[i],i&255,n);}
    for(unsigned i=0;i<16384;i++){size_t n=1+i%1536;unsigned char *p=nps_realloc(blocks[i],n+73);assert(p);for(size_t j=0;j<n;j++)assert(p[j]==(i&255));nps_free(p);}
    for(unsigned i=0;i<1000;i++){void *p;assert(!nps_posix_memalign(&p,16384,32768));assert((uintptr_t)p%16384==0);memset(p,0xff,32768);nps_free(p);}
    unsigned char *canvas=nps_calloc(1280*720,4);assert(canvas);
    for(size_t i=0;i<1280*720*4;i++)assert(canvas[i]==0);
    memset(canvas,0xa5,1280*720*4);nps_free(canvas);
    canvas=nps_calloc(1280*720,4);assert(canvas);
    for(size_t i=0;i<1280*720*4;i++)assert(canvas[i]==0);
    nps_free(canvas);
    assert(!nps_calloc(SIZE_MAX,2) && errno==ENOMEM);
    void *p=NULL;assert(nps_posix_memalign(&p,3,16)==EINVAL);
    assert(map_count==1);free(reservation);
    puts("PASS: dlmalloc 16384 allocations/reallocations, alignment, overflow, guarded heap mapping and reservation denial");
}
