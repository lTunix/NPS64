/* Exercise the actual core's opt-in voice coefficient banks. */
#include "resample_hq.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
static double response(const int16_t *k,unsigned taps,double frequency){
 double re=0,im=0;for(unsigned j=0;j<taps;j++){double a=2*3.14159265358979323846*frequency*j;re+=k[j]/32768.0*cos(a);im+=k[j]/32768.0*sin(a);}return sqrt(re*re+im*im);
}
int main(void){
 resample_hq_set_quality(0);assert(!resample_hq_enabled()&&!resample_hq_taps(0x20000,0));
 resample_hq_set_quality(32);assert(resample_hq_enabled());
 const int16_t *capped=resample_hq_taps_capped(0x10000,0,16);assert(capped&&resample_hq_taps_count()==16);
 const int16_t *saved=resample_hq_taps(0x20000,0);assert(saved&&resample_hq_taps_count()==32);
 double pass=response(saved,32,.1),stop=response(saved,32,.4);
 assert(pass>.95&&pass<1.05&&stop<.01);
 for(unsigned p=0;p<256;p++){
  const int16_t *k=resample_hq_taps(0x20000,p<<8);long sum=0;for(unsigned j=0;j<32;j++)sum+=k[j];assert(sum==32768);
 }
 assert(resample_hq_taps_capped(0x10000,0,16)==capped);
 assert(resample_hq_taps(0x20000,0)==saved); /* Switching widths must reuse the tables. */
 resample_hq_release();assert(!resample_hq_enabled());
 printf("PASS HLE voice filter: disabled fallback, 16/32 banks, unity gain, reuse, passband=%.6f stopband=%.6f\n",pass,stop);return 0;
}
