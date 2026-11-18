#include "rdp_emit_rsp.h"
#include <stdint.h>
#include <stdio.h>
extern int32_t nps64_reference_rsp_rcp32(int32_t);
extern int32_t nps64_reference_rsp_rcp32_dp(int32_t);
extern int32_t nps64_reference_rsp_rcp16(int32_t);
extern int32_t nps64_reference_rsp_rsq32(int32_t);
static uint32_t rng=0x45e26a31;
static unsigned long cases;
static int check(int32_t x) {
    int32_t a[]={rsp_rcp32(x),rsp_rcp32_dp(x),rsp_rcp16(x),rsp_rsq32(x)};
    int32_t b[]={nps64_reference_rsp_rcp32(x),nps64_reference_rsp_rcp32_dp(x),nps64_reference_rsp_rcp16(x),nps64_reference_rsp_rsq32(x)};
    for(unsigned i=0;i<4;i++)if(a[i]!=b[i]){
        fprintf(stderr,"FAIL reciprocal function=%u x=%08x actual=%08x expected=%08x\n",i,(unsigned)x,(unsigned)a[i],(unsigned)b[i]);return 1;
    }
    cases+=4;return 0;
}
int main(void) {
    for(int x=-32768;x<=32767;x++)if(check(x))return 1;
    for(unsigned bit=0;bit<32;bit++)for(int d=-2;d<=2;d++){
        uint32_t x=(UINT32_C(1)<<bit)+(uint32_t)d;
        if(check((int32_t)x)||check((int32_t)(0u-x)))return 1;
    }
    for(unsigned i=0;i<1000000;i++){
        rng^=rng<<13;rng^=rng>>17;rng^=rng<<5;
        if(check((int32_t)rng))return 1;
    }
    printf("PASS reciprocal normalization: %lu exact outputs, exhaustive 16-bit, signed boundaries and 32-bit random inputs\n",cases);
}
