#include "audio_stretch.h"
#include <assert.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
static Nps64AudioQueue q;static Nps64AudioStretch s;static int16_t block[NPS64_AUDIO_BLOCK*2];
static unsigned source;static double phase,frequency=997;
static void produce(unsigned n){for(unsigned i=0;i<n;i++){int16_t v=(int16_t)(12000*sin(phase));phase+=2*3.14159265358979323846*frequency/48000;assert(nps64_audio_push(&q,v,(int16_t)-v));source++;}}
int main(void){
 setbuf(stdout,NULL);
 const double frequencies[]={100,997,4000};
 for(unsigned tone=0;tone<3;tone++)for(unsigned profile=0;profile<4;profile++){
  frequency=frequencies[tone];
  memset(&q,0,sizeof q);memset(&s,0,sizeof s);phase=0;source=0;produce(NPS64_AUDIO_PREFILL);
  double speed=profile==0?1:profile==1?.92:profile==2?1.02:.97,carry=0;unsigned crossings=0,measured=0;int16_t last=0;int maxjump=0;
  for(unsigned k=0;k<2400;k++){
   if(k){carry+=NPS64_AUDIO_BLOCK*speed;unsigned n=(unsigned)carry;carry-=n;produce(n);}
   assert(nps64_audio_take_smooth(&q,&s,block)==NPS64_AUDIO_BLOCK);
   if(k>100){for(unsigned i=0;i<NPS64_AUDIO_BLOCK;i++){int16_t v=block[i*2];assert(abs((int)v+block[i*2+1])<=1);if(v>=0&&last<0)crossings++;int jump=abs((int)v-last);if(jump>maxjump)maxjump=jump;last=v;measured++;}}else last=block[(NPS64_AUDIO_BLOCK-1)*2];
  }
  double hz=crossings*48000.0/measured;printf("speed=%.2f frequency=%.3f max_step=%d underruns=%u dropped=%u tempo=%.5f queued=%u\n",speed,hz,maxjump,q.underruns,q.dropped,s.tempo,q.write-q.read);
  assert(fabs(hz-frequency)<3&&maxjump<2*12000*sin(3.14159265358979323846*frequency/48000)+1500&&!q.underruns&&!q.dropped);
  while(nps64_audio_take_smooth(&q,&s,block)){}assert(q.underruns==1);for(unsigned i=128;i<NPS64_AUDIO_BLOCK;i++)assert(block[i*2]==0);
  nps64_audio_reset(&q);assert(!nps64_audio_take_smooth(&q,&s,block));produce(NPS64_AUDIO_PREFILL);assert(nps64_audio_take_smooth(&q,&s,block));
 }
 memset(&q,0,sizeof q);memset(&s,0,sizeof s);phase=0;produce(NPS64_AUDIO_PREFILL);
 unsigned batch=0,output=0;
 for(unsigned us=0;us<20000000;us+=100){
  while(us>=batch*20000+(batch%2?8000:0)+((batch%100==50)?40000:0)){produce(960);batch++;}
  if(us>=output*64000/3){assert(nps64_audio_take_smooth(&q,&s,block));output++;}
 }
 assert(!q.underruns&&!q.dropped);
 puts("PASS pitch-preserving audio: sustained slowdown, device drift, bounded FIFO, stereo, underflow fade, reset and restart");return 0;
}
