/* Same consumer and arrival schedule for the checkpoint and larger profile. */
#include "audio_stretch.h"
#include <assert.h>
#include <stdio.h>
#include <math.h>
static Nps64AudioQueue q;static Nps64AudioStretch s;
static int16_t block[NPS64_AUDIO_BLOCK*2];
static unsigned sample;
static void produce(unsigned frames){
 for(unsigned i=0;i<frames;i++){
  int16_t v=(int16_t)(10000*sin(sample++*(2*3.14159265358979323846*997/48000)));
  nps64_audio_push(&q,v,(int16_t)-v);
 }
}
int main(void){
 /* No playback before the requested reserve has really been produced. */
 produce(NPS64_AUDIO_PREFILL-1);assert(!nps64_audio_take_smooth(&q,&s,block));
 produce(1);assert(nps64_audio_take_smooth(&q,&s,block));
 memset(&q,0,sizeof q);memset(&s,0,sizeof s);sample=0;produce(NPS64_AUDIO_PREFILL);
 unsigned batch=0,output=0;
 for(unsigned us=0;us<20000000;us+=100){
  /* PAL 20 ms fields, +/-8 ms jitter, periodic 100 ms producer stalls. */
  while(us>=batch*20000+(batch%2?8000:0)+(batch%100==50?100000:0)){produce(960);batch++;}
  if(us>=output*64000/3){nps64_audio_take_smooth(&q,&s,block);output++;}
  assert(q.write-q.read<=NPS64_AUDIO_CAPACITY);
 }
 printf("capacity=%u prefill=%u target=%u underruns=%u dropped=%u\n",NPS64_AUDIO_CAPACITY,NPS64_AUDIO_PREFILL,NPS64_AUDIO_TARGET,q.underruns,q.dropped);
#ifdef NPS64_AUDIO_BUFFER_TEST
 assert(!q.underruns&&!q.dropped);
#else
 assert(q.underruns>0);
#endif
 puts("PASS A/B audio reserve: actual prefill, periodic 100 ms stalls, bounded FIFO");return 0;
}
