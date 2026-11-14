/* A silent voice must not depend on neighboring DMEM scratch data. */
#include "alist.h"
#include "hle_internal.h"
#include "resample_hq.h"
#include "memory.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static struct hle_t hle;
static unsigned char ram[4096];
void HleWarnMessage(void *u,const char *message,...){(void)u;(void)message;}
static int run(int taps,int16_t scratch){
 memset(&hle,0,sizeof(hle));memset(ram,0,sizeof(ram));hle.dram=ram;
 int16_t *samples=(int16_t*)hle.alist_buffer;
 for(unsigned i=0;i<2048;i++)samples[i]=scratch;
 /* Four restored history samples and four silent source samples. */
 for(unsigned i=508;i<516;i++)samples[i^S]=0;
 resample_hq_set_quality(taps);
 alist_resample(&hle,1,0,0x900,0x400,2,0x12000,0x100);
 return samples[(0x900/2)^S];
}
int main(void){
 int exact_a=run(0,0),exact_b=run(0,12000);
 int enhanced_a=run(32,0),enhanced_b=run(32,12000);
 assert(exact_a==0&&exact_b==0);
 printf("Voice isolation: exact=%d/%d enhanced32=%d/%d (only neighboring scratch changes)\n",exact_a,exact_b,enhanced_a,enhanced_b);
 assert(enhanced_a!=enhanced_b);
 resample_hq_release();
 puts("PASS diagnosis: extended voice filter imports neighboring scratch into silent voice");
}
