#ifndef NPS64_AUDIO_QUEUE_H
#define NPS64_AUDIO_QUEUE_H
#include <stdint.h>
#include <stddef.h>
#include <string.h>
enum { NPS64_AUDIO_CAPACITY=8192, NPS64_AUDIO_BLOCK=1024, NPS64_AUDIO_PREFILL=5760 };
typedef struct {
    int16_t samples[NPS64_AUDIO_CAPACITY*2];
    _Alignas(64) unsigned write;
    _Alignas(64) unsigned read;
    unsigned reset,seen_reset,underruns,dropped;
    int playing;
} Nps64AudioQueue;
/* Producer owns write/reset; consumer owns read and playback state. */
static inline void nps64_audio_reset(Nps64AudioQueue *q) {
    __atomic_add_fetch(&q->reset,1,__ATOMIC_RELEASE);
}
/* Publish once per batch, not once per stereo sample. */
static inline size_t nps64_audio_push_batch(Nps64AudioQueue *q,const int16_t *samples,size_t frames) {
    unsigned w=__atomic_load_n(&q->write,__ATOMIC_RELAXED);
    unsigned rd=__atomic_load_n(&q->read,__ATOMIC_ACQUIRE);
    unsigned available=NPS64_AUDIO_CAPACITY-(w-rd);
    size_t count=frames<available?frames:available;
    size_t first=NPS64_AUDIO_CAPACITY-w%NPS64_AUDIO_CAPACITY;
    if(first>count)first=count;
    if(first)memcpy(q->samples+(w%NPS64_AUDIO_CAPACITY)*2,samples,first*4);
    if(count>first)memcpy(q->samples,samples+first*2,(count-first)*4);
    if(count)__atomic_store_n(&q->write,w+(unsigned)count,__ATOMIC_RELEASE);
    if(frames>count)__atomic_add_fetch(&q->dropped,(unsigned)(frames-count),__ATOMIC_RELAXED);
    return count;
}
static inline int nps64_audio_push(Nps64AudioQueue *q,int16_t l,int16_t r) {
    int16_t sample[2]={l,r};return nps64_audio_push_batch(q,sample,1)==1;
}
static inline unsigned nps64_audio_take(Nps64AudioQueue *q,int16_t *block) {
    unsigned rd=__atomic_load_n(&q->read,__ATOMIC_RELAXED);
    unsigned w=__atomic_load_n(&q->write,__ATOMIC_ACQUIRE);
    unsigned reset=__atomic_load_n(&q->reset,__ATOMIC_ACQUIRE);
    if(reset!=q->seen_reset) {
        q->seen_reset=reset;q->playing=0;rd=w;
        __atomic_store_n(&q->read,rd,__ATOMIC_RELEASE);
    }
    unsigned count=w-rd;
    if(!q->playing && count>=NPS64_AUDIO_PREFILL)q->playing=1;
    if(q->playing && count<NPS64_AUDIO_BLOCK) {
        q->playing=0;__atomic_add_fetch(&q->underruns,1,__ATOMIC_RELAXED);
    }
    if(!q->playing) {memset(block,0,NPS64_AUDIO_BLOCK*4);return 0;}
    unsigned index=rd%NPS64_AUDIO_CAPACITY;
    unsigned first=NPS64_AUDIO_CAPACITY-index;
    if(first>NPS64_AUDIO_BLOCK)first=NPS64_AUDIO_BLOCK;
    memcpy(block,q->samples+index*2,first*4);
    if(first<NPS64_AUDIO_BLOCK)memcpy(block+first*2,q->samples,(NPS64_AUDIO_BLOCK-first)*4);
    __atomic_store_n(&q->read,rd+NPS64_AUDIO_BLOCK,__ATOMIC_RELEASE);
    return NPS64_AUDIO_BLOCK;
}
#endif
