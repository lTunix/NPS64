#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#define MB ((size_t)1024*1024)
void *nps_malloc(size_t);void nps_free(void*);
size_t ps4rt_heap_size(void);const char *ps4rt_heap_source(void);
static size_t available=4608*MB,limit=512*MB,expected=512*MB;
static int map_fail,released,attempts;
size_t sceKernelGetDirectMemorySize(void){return 6144*MB;}
int sceKernelAvailableDirectMemorySize(int64_t a,int64_t b,size_t align,int64_t *p,size_t *n){(void)a;(void)b;assert(align==2*MB);*p=0;*n=available;return 0;}
int sceKernelAllocateDirectMemory(int64_t a,int64_t b,size_t n,size_t align,int type,int64_t *p){(void)a;(void)b;assert(n+2560*MB<=available && n<=512*MB && align==2*MB && type==0);attempts++;*p=0;return n>limit?-1:0;}
int sceKernelMapDirectMemory(void **a,size_t n,int prot,int flags,int64_t p,size_t align){assert(prot==3 && flags==0 && p==0 && align==2*MB);if(map_fail && n==512*MB)return -1;assert(n==expected);*a=(void*)(uintptr_t)0x10000000;return 0;}
int sceKernelReleaseDirectMemory(int64_t p,size_t n){assert(p==0 && n==512*MB);released++;return 0;}
int sceKernelAvailableFlexibleMemorySize(size_t *n){*n=0;return 0;}
int sceKernelMapNamedFlexibleMemory(void **a,size_t n,int p,int f,const char *name){(void)a;(void)n;(void)p;(void)f;(void)name;assert(0);return -1;}
void *sceLibcMspaceCreate(const char *s,void *base,size_t capacity,unsigned flags){assert(!strcmp(s,"NPS64") && base==(void*)(uintptr_t)0x10000000 && capacity==expected && flags==0);return base;}
void *sceLibcMspaceMalloc(void *s,size_t n){assert(s==(void*)(uintptr_t)0x10000000);return malloc(n);}
int sceLibcMspaceFree(void *s,void *p){assert(s==(void*)(uintptr_t)0x10000000);free(p);return 0;}
int main(int argc,char **argv){
 assert(argc==2);
 if(!strcmp(argv[1],"allocation-fail")){limit=expected=256*MB;}
 if(!strcmp(argv[1],"map-fail")){map_fail=1;expected=256*MB;}
 if(!strcmp(argv[1],"limited")){available=2816*MB;expected=256*MB;}
 void *p=nps_malloc(2048);assert(p && (uintptr_t)p%16==0);memset(p,0x42,2048);nps_free(p);
 assert(ps4rt_heap_size()==expected && !strcmp(ps4rt_heap_source(),"direct memory"));
 assert(attempts==((limit<512*MB || map_fail)?2:1));assert(released==map_fail);
 printf("PASS CPU heap %s: %zu MiB, reserve intact, fallback verified\n",argv[1],expected/MB);
}
