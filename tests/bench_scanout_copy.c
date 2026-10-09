#define _POSIX_C_SOURCE 200809L
#include "scanout_copy.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
extern void *nps64_sdk_memcpy(void *,const void *,size_t);
static double seconds(void){struct timespec t;clock_gettime(CLOCK_MONOTONIC,&t);return t.tv_sec+t.tv_nsec*1e-9;}
int main(void){
    const size_t size=1280*720*4;
    void *src,*dst;
    assert(!posix_memalign(&src,64,size)&&!posix_memalign(&dst,64,size));
    memset(src,0x79,size);memset(dst,0,size);
    for(unsigned round=0;round<6;round++)for(unsigned trial=0;trial<2;trial++){
        unsigned streaming=(round+trial)%2;
        double start=seconds();
        for(unsigned i=0;i<300;i++){
            if(streaming)nps64_scanout_copy(dst,src,size);
            else {nps64_sdk_memcpy(dst,src,size);_mm_sfence();}
        }
        double ms=(seconds()-start)*1000/300;
        assert(!memcmp(dst,src,size));
        printf("COPY round=%u streaming=%u ms=%.6f\n",round,streaming,ms);
    }
    free(src);free(dst);
}
