#include "audio_queue.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
static Nps64AudioQueue q;
static int16_t block[NPS64_AUDIO_BLOCK*2];
static unsigned sequence;
static void push(unsigned n) {
    for(unsigned i=0;i<n;i++) {
        int16_t v=(int16_t)sequence++;
        assert(nps64_audio_push(&q,v,(int16_t)~v));
    }
}
static unsigned expected;
static void take(void) {
    assert(nps64_audio_take(&q,block)==NPS64_AUDIO_BLOCK);
    for(unsigned i=0;i<NPS64_AUDIO_BLOCK;i++) {
        int16_t v=(int16_t)expected++;
        assert(block[i*2]==v && block[i*2+1]==(int16_t)~v);
    }
}
int main(void) {
    assert(nps64_audio_take(&q,block)==0);
    push(NPS64_AUDIO_PREFILL-1);
    assert(nps64_audio_take(&q,block)==0 && q.read==0);
    push(1);take();
    for(unsigned i=0;i<1000;i++){push(1024);take();}
    while(q.write-q.read>=NPS64_AUDIO_BLOCK)take();
    assert(nps64_audio_take(&q,block)==0 && q.underruns==1);
    push(NPS64_AUDIO_PREFILL-(q.write-q.read)-1);assert(nps64_audio_take(&q,block)==0);
    push(1);take();
    nps64_audio_reset(&q);assert(nps64_audio_take(&q,block)==0 && q.read==q.write);
    expected=sequence;push(NPS64_AUDIO_PREFILL);take();
    while(q.write-q.read<NPS64_AUDIO_CAPACITY)push(1);
    assert(!nps64_audio_push(&q,7,8) && q.dropped==1);
    memset(&q,0,sizeof(q));q.read=q.write=UINT_MAX-1023;
    sequence=expected=0;push(NPS64_AUDIO_PREFILL);take();take();take();
    memset(&q,0,sizeof(q));q.read=q.write=NPS64_AUDIO_CAPACITY-513;
    sequence=expected=0;
    int16_t samples_batch[256*2];
    for(unsigned k=0;k<24;k++) {
        for(unsigned i=0;i<256;i++) {int16_t v=(int16_t)sequence++;samples_batch[i*2]=v;samples_batch[i*2+1]=(int16_t)~v;}
        assert(nps64_audio_push_batch(&q,samples_batch,256)==256);
    }
    for(unsigned k=0;k<6;k++)take();
    assert(!q.dropped);
    /* PAL batches every 20 ms with alternating +/- 8 ms delivery jitter.
       Device requests 1024 samples every 21.333 ms, for 20 seconds. */
    memset(&q,0,sizeof(q));sequence=expected=0;
    unsigned batch=0,output=0;
    for(unsigned us=0;us<20000000;us+=100) {
        while(us>=batch*20000+(batch%2?8000:0)+((batch%100==50)?40000:0)){push(960);batch++;}
        if(us>=output*64000/3){
            unsigned n=nps64_audio_take(&q,block);
            if(n) {
                for(unsigned i=0;i<n;i++)assert(block[i*2]==(int16_t)expected++);
            }
            output++;
        }
    }
    assert(q.underruns==0 && q.dropped==0);
    puts("PASS audio: prefill, jitter + periodic 40 ms delivery stalls, ordered samples, rebuffer, reset, overflow, counter wrap");
}
