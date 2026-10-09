#include "audio_resampler.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
static Nps64AudioResampler a,b;
static int16_t input[192000*2],one[300000*2],chunked[300000*2];
typedef struct {int16_t *samples;size_t count;} Sink;
static void emit(void *p,int16_t l,int16_t r) {
 Sink *s=p;assert(s->count<300000);s->samples[s->count*2]=l;s->samples[s->count*2+1]=r;s->count++;
}
static double amplitude(const int16_t *s,size_t n,double hz) {
 double re=0,im=0,pi=3.14159265358979323846;
 for(size_t i=128;i<n;i++){double t=2*pi*hz*i/48000;re+=s[i*2]*cos(t);im+=s[i*2]*sin(t);}
 return 2*sqrt(re*re+im*im)/(n-128);
}
int main(void) {
 const double rates[]={8000,22037.94,32000,32040,44100,48000,96000,192000};
 for(unsigned k=0;k<sizeof(rates)/sizeof(rates[0]);k++) {
  double rate=rates[k];size_t n=(size_t)rate;
  for(size_t i=0;i<n;i++){input[i*2]=(int16_t)((i*391u)%60001u-30000);input[i*2+1]=(int16_t)-input[i*2];}
  memset(&a,0,sizeof(a));memset(&b,0,sizeof(b));Sink x={one,0},y={chunked,0};
  nps64_resample(&a,input,n,rate,emit,&x);
  size_t offset=0;while(offset<n){size_t c=1+(offset*13)%997;if(c>n-offset)c=n-offset;nps64_resample(&b,input+offset*2,c,rate,emit,&y);offset+=c;}
  assert(x.count==y.count && !memcmp(one,chunked,x.count*4));
  assert(fabs((double)x.count-(double)n*48000/rate)<=1.01);
 }
 memset(&a,0,sizeof(a));for(size_t i=0;i<32040;i++){input[i*2]=12345;input[i*2+1]=-23456;}
 Sink x={one,0};nps64_resample(&a,input,32040,32040,emit,&x);
 for(size_t i=0;i<x.count;i++)assert(one[i*2]==12345 && one[i*2+1]==-23456);
 nps64_resampler_reset(&a);input[0]=77;input[1]=-99;x.count=0;nps64_resample(&a,input,1,32040,emit,&x);
 for(size_t i=0;i<x.count;i++)assert(one[i*2]==77 && one[i*2+1]==-99);
 double pi=3.14159265358979323846;
 for(size_t i=0;i<32040;i++)input[i*2]=input[i*2+1]=(int16_t)(16000*sin(2*pi*1000*i/32040));
 memset(&a,0,sizeof(a));x.count=0;nps64_resample(&a,input,32040,32040,emit,&x);
 double error=0;for(size_t i=128;i<x.count;i++){
  double ref=16000*sin(2*pi*1000*((double)i/48000-15.0/32040));
  double e=one[i*2]-ref;error+=e*e;
 }
 double rms=sqrt(error/(x.count-128));assert(rms<20);
 for(size_t i=0;i<32040;i++)input[i*2]=input[i*2+1]=(int16_t)(16000*sin(2*pi*10000*i/32040));
 memset(&a,0,sizeof(a));x.count=0;nps64_resample(&a,input,32040,32040,emit,&x);
 double fundamental=amplitude(one,x.count,10000),image=amplitude(one,x.count,22040);
 assert(fundamental>15000 && image/fundamental<0.01);
 double phase=0;size_t old_count=0;while(phase<32040){size_t i=(size_t)phase;chunked[old_count*2]=input[i*2];old_count++;phase+=32040.0/48000;}
 double old_image=amplitude(chunked,old_count,22040)/amplitude(chunked,old_count,10000);
 assert(image/fundamental<old_image/20);
 const char *dump=getenv("NPS64_AUDIO_TEST_DUMP");
 if(dump){FILE *f=fopen(dump,"wb");assert(f);assert(fwrite(one,4,x.count,f)==x.count);assert(!fclose(f));}
 clock_t begin=clock();for(unsigned i=0;i<100;i++){x.count=0;nps64_resample(&a,input,1024,32040,emit,&x);}
 printf("PASS resampler: batch invariance, rates, DC/stereo, reset, RMS=%.3f, image ratio %.6f (nearest %.6f), cost %.3f ms / 1024 input samples\n",rms,image/fundamental,old_image,1000.0*(clock()-begin)/CLOCKS_PER_SEC/100);
}
