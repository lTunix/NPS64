#include "audio_queue.h"
#include <assert.h>
#include <limits.h>
#include <stdio.h>
static Nps64AudioQueue q,old,newq;
static int16_t out[NPS64_AUDIO_BLOCK*2],samples[960*2];
static void push(Nps64AudioQueue *s,unsigned count){while(count){unsigned n=count>960?960:count;assert(nps64_audio_push_batch(s,samples,n)==n);count-=n;}}
static Nps64AudioQueue ordered;
static unsigned next_sample,expected_sample;
static void push_ordered(unsigned count){
 int16_t data[256*2];
 while(count){unsigned n=count>256?256:count;
  for(unsigned i=0;i<n;i++,next_sample++){data[i*2]=(int16_t)(next_sample*73+19);data[i*2+1]=(int16_t)((next_sample^0x1a75u)*31);}
  assert(nps64_audio_push_batch(&ordered,data,n)==n);count-=n;
 }
}
int main(void){
 ordered.read=ordered.write=UINT_MAX-512;push_ordered(NPS64_AUDIO_PREFILL);
 for(unsigned b=0;b<1000;b++){
  assert(nps64_audio_take_recover(&ordered,out)==NPS64_AUDIO_BLOCK);
  for(unsigned i=0;i<NPS64_AUDIO_BLOCK;i++,expected_sample++){
   assert(out[2*i]==(int16_t)(expected_sample*73+19));
   assert(out[2*i+1]==(int16_t)((expected_sample^0x1a75u)*31));
  }
  push_ordered(NPS64_AUDIO_BLOCK);
 }
 assert(!ordered.underruns&&!ordered.dropped);
 for(unsigned i=0;i<960;i++){samples[2*i]=10000;samples[2*i+1]=-6000;}
 assert(nps64_audio_take_recover(&q,out)==0);push(&q,NPS64_AUDIO_PREFILL-1);assert(!nps64_audio_take_recover(&q,out));push(&q,1);
 for(unsigned b=0;b<1000;b++){
  assert(nps64_audio_take_recover(&q,out)==NPS64_AUDIO_BLOCK);
  for(unsigned i=0;i<NPS64_AUDIO_BLOCK;i++)assert(out[2*i]==10000&&out[2*i+1]==-6000);
  push(&q,NPS64_AUDIO_BLOCK);
 }
 while(q.write-q.read>=NPS64_AUDIO_BLOCK)assert(nps64_audio_take_recover(&q,out)==NPS64_AUDIO_BLOCK);
 unsigned tail=q.write-q.read;assert(tail&&tail<NPS64_AUDIO_BLOCK);
 assert(nps64_audio_take_recover(&q,out)==tail&&q.read==q.write&&q.underruns==1&&q.partial_frames==tail);
 for(unsigned i=0;i<tail-NPS64_AUDIO_FADE;i++)assert(out[2*i]==10000);
 assert(out[2*(tail-1)]==0);for(unsigned i=tail;i<NPS64_AUDIO_BLOCK;i++)assert(!out[2*i]&&!out[2*i+1]);
 push(&q,NPS64_AUDIO_RESTART-1);assert(!nps64_audio_take_recover(&q,out));push(&q,1);assert(nps64_audio_take_recover(&q,out)==NPS64_AUDIO_BLOCK);
 assert(!out[0]&&out[2*(NPS64_AUDIO_FADE-1)]==10000&&out[2*NPS64_AUDIO_FADE]==10000);
 nps64_audio_reset(&q);assert(!nps64_audio_take_recover(&q,out)&&q.read==q.write&&!q.recovering);
 push(&q,NPS64_AUDIO_RESTART);assert(!nps64_audio_take_recover(&q,out));push(&q,NPS64_AUDIO_PREFILL-NPS64_AUDIO_RESTART);assert(nps64_audio_take_recover(&q,out)==NPS64_AUDIO_BLOCK);
 memset(&q,0,sizeof(q));q.read=q.write=UINT_MAX-512;push(&q,NPS64_AUDIO_PREFILL);assert(nps64_audio_take_recover(&q,out)==NPS64_AUDIO_BLOCK);
 /* One 360 ms producer stall followed by normal delivery: compare the
  * longest all-zero output run, excluding initial startup, at equal input. */
 unsigned source=0,device=0,oldrun=0,newrun=0,oldmax=0,newmax=0;
 for(unsigned us=0;us<12000000;us+=100){
  if(us>=source*20000u){if(us<5000000||us>=5360000){push(&old,960);push(&newq,960);}source++;}
  if(us>=device*64000u/3){
   unsigned a=nps64_audio_take(&old,out);if(us>1000000){oldrun=a?0:oldrun+1;if(oldrun>oldmax)oldmax=oldrun;}
   unsigned b=nps64_audio_take_recover(&newq,out);if(us>1000000){newrun=b?0:newrun+1;if(newrun>newmax)newmax=newrun;}
   device++;
  }
 }
 assert(old.underruns==1&&newq.underruns==1&&newmax<oldmax&&!old.dropped&&!newq.dropped);
 printf("PASS audio recovery: healthy exact samples, partial tail, 64-sample fades, 85 ms restart, 240 ms intentional reset, wrap; isolated stall zero-run blocks %u -> %u, same underrun count\n",oldmax,newmax);
}
