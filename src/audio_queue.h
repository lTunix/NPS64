#ifndef NPS64_AUDIO_QUEUE_H
#define NPS64_AUDIO_QUEUE_H
#include <stdint.h>
#include <stddef.h>
#include <string.h>
/* 00.18 isolates a larger reserve for the console comparison. Normal builds
 * retain the checkpoint's buffer sizes; the device output block is identical. */
#ifdef NPS64_AUDIO_BUFFER_TEST
enum { NPS64_AUDIO_CAPACITY=16384, NPS64_AUDIO_BLOCK=1024, NPS64_AUDIO_PREFILL=11520, NPS64_AUDIO_TARGET=10240 };
#else
enum { NPS64_AUDIO_CAPACITY=8192, NPS64_AUDIO_BLOCK=1024, NPS64_AUDIO_PREFILL=5760, NPS64_AUDIO_TARGET=5120 };
#endif
typedef struct {
    int16_t samples[NPS64_AUDIO_CAPACITY*2];
    _Alignas(64) unsigned write;
    _Alignas(64) unsigned read;
    unsigned reset,seen_reset,underruns,dropped;
    int playing;
    int recovering,fade_in;
    int16_t last_left,last_right;
    unsigned partial_frames,recovery_frames,recovery_current,recovery_max;
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
/* Recovery keeps the original start reserve, but does not require another
 * full 240 ms after every underrun. Healthy playback remains byte-identical.
 * Only the missing output is padded; consume the partial tail before waiting. */
enum { NPS64_AUDIO_RESTART=4096,NPS64_AUDIO_FADE=64 };
static inline void nps64_audio_recovery_gap(Nps64AudioQueue *q,unsigned frames){
    __atomic_add_fetch(&q->recovery_frames,frames,__ATOMIC_RELAXED);
    unsigned current=__atomic_add_fetch(&q->recovery_current,frames,__ATOMIC_RELAXED);
    if(current>__atomic_load_n(&q->recovery_max,__ATOMIC_RELAXED))__atomic_store_n(&q->recovery_max,current,__ATOMIC_RELAXED);
}
static inline unsigned nps64_audio_take_recover(Nps64AudioQueue *q,int16_t *block){
    unsigned rd=__atomic_load_n(&q->read,__ATOMIC_RELAXED),w=__atomic_load_n(&q->write,__ATOMIC_ACQUIRE);
    unsigned reset=__atomic_load_n(&q->reset,__ATOMIC_ACQUIRE);
    if(reset!=q->seen_reset){
        q->seen_reset=reset;q->playing=q->recovering=q->fade_in=0;q->last_left=q->last_right=0;rd=w;
        __atomic_store_n(&q->recovery_current,0,__ATOMIC_RELAXED);__atomic_store_n(&q->read,rd,__ATOMIC_RELEASE);
    }
    unsigned count=w-rd;
    if(!q->playing && count>=(q->recovering?NPS64_AUDIO_RESTART:NPS64_AUDIO_PREFILL)){
        q->playing=1;q->fade_in=q->recovering;q->recovering=0;__atomic_store_n(&q->recovery_current,0,__ATOMIC_RELAXED);
    }
    if(!q->playing){memset(block,0,NPS64_AUDIO_BLOCK*4);if(q->recovering)nps64_audio_recovery_gap(q,NPS64_AUDIO_BLOCK);return 0;}
    unsigned frames=count<NPS64_AUDIO_BLOCK?count:NPS64_AUDIO_BLOCK;
    unsigned index=rd%NPS64_AUDIO_CAPACITY,first=NPS64_AUDIO_CAPACITY-index;
    if(first>frames)first=frames;
    memcpy(block,q->samples+index*2,first*4);
    if(first<frames)memcpy(block+first*2,q->samples,(frames-first)*4);
    if(q->fade_in){
        unsigned fade=frames<NPS64_AUDIO_FADE?frames:NPS64_AUDIO_FADE;
        for(unsigned i=0;i<fade;i++)for(unsigned c=0;c<2;c++)block[i*2+c]=(int16_t)((int)block[i*2+c]*(int)i/(int)(fade>1?fade-1:1));
        q->fade_in=0;
    }
    if(frames<NPS64_AUDIO_BLOCK){
        memset(block+frames*2,0,(NPS64_AUDIO_BLOCK-frames)*4);
        unsigned fade=frames<NPS64_AUDIO_FADE?frames:NPS64_AUDIO_FADE;
        for(unsigned i=0;i<fade;i++)for(unsigned c=0;c<2;c++){
            unsigned at=frames-fade+i;block[at*2+c]=(int16_t)((int)block[at*2+c]*(int)(fade-1-i)/(int)(fade>1?fade-1:1));
        }
        if(!frames)for(unsigned i=0;i<NPS64_AUDIO_FADE;i++){
            block[i*2]=(int16_t)((int)q->last_left*(NPS64_AUDIO_FADE-1-(int)i)/(NPS64_AUDIO_FADE-1));
            block[i*2+1]=(int16_t)((int)q->last_right*(NPS64_AUDIO_FADE-1-(int)i)/(NPS64_AUDIO_FADE-1));
        }
        q->playing=0;q->recovering=1;
        __atomic_add_fetch(&q->underruns,1,__ATOMIC_RELAXED);__atomic_add_fetch(&q->partial_frames,frames,__ATOMIC_RELAXED);
        nps64_audio_recovery_gap(q,NPS64_AUDIO_BLOCK-frames);
    }
    q->last_left=block[(NPS64_AUDIO_BLOCK-1)*2];q->last_right=block[(NPS64_AUDIO_BLOCK-1)*2+1];
    __atomic_store_n(&q->read,rd+frames,__ATOMIC_RELEASE);return frames;
}
#endif
