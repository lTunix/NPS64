/* Bounded overlap-add rate matching for the PS4 audio clock. GPL-2.0-or-later */
#ifndef NPS64_AUDIO_STRETCH_H
#define NPS64_AUDIO_STRETCH_H
#include "audio_queue.h"
/* Keep timing correction in the consumer: video and the emulated DAC keep
 * their original clocks. A fixed analysis hop preserves the average tempo;
 * correlation chooses only the waveform alignment, never the FIFO advance. */
enum {NPS64_AUDIO_SEARCH=384,NPS64_AUDIO_TARGET=5120};
typedef struct {
    int16_t tail[NPS64_AUDIO_BLOCK*2],last[2];
    double level,tempo,fraction;
    unsigned seen_reset;
    int valid;
} Nps64AudioStretch;
static inline int16_t nps64_audio_at(const Nps64AudioQueue *q,unsigned rd,unsigned frame,unsigned channel){return q->samples[((rd+frame)%NPS64_AUDIO_CAPACITY)*2+channel];}
static inline uint64_t nps64_audio_match(const Nps64AudioQueue *q,const Nps64AudioStretch *s,unsigned rd,unsigned offset){
    uint64_t error=0;
    for(unsigned i=0;i<NPS64_AUDIO_BLOCK;i+=16)for(unsigned c=0;c<2;c++){
        int64_t d=(int)s->tail[i*2+c]-nps64_audio_at(q,rd,offset+i,c);error+=(uint64_t)(d*d);
    }
    return error;
}
static inline unsigned nps64_audio_take_smooth(Nps64AudioQueue *q,Nps64AudioStretch *s,int16_t *block){
    unsigned rd=__atomic_load_n(&q->read,__ATOMIC_RELAXED),w=__atomic_load_n(&q->write,__ATOMIC_ACQUIRE);
    unsigned reset=__atomic_load_n(&q->reset,__ATOMIC_ACQUIRE);
    if(reset!=s->seen_reset){s->seen_reset=reset;q->seen_reset=reset;q->playing=0;s->valid=0;s->last[0]=s->last[1]=0;rd=w;__atomic_store_n(&q->read,rd,__ATOMIC_RELEASE);}
    unsigned count=w-rd;
    if(!q->playing && count>=NPS64_AUDIO_PREFILL){q->playing=1;s->valid=0;s->tempo=1;s->level=NPS64_AUDIO_TARGET;s->fraction=0;}
    if(q->playing && count<2*NPS64_AUDIO_BLOCK+2*NPS64_AUDIO_SEARCH){q->playing=0;__atomic_add_fetch(&q->underruns,1,__ATOMIC_RELAXED);}
    if(!q->playing){
        memset(block,0,NPS64_AUDIO_BLOCK*4);
        /* A short fade avoids a discontinuity when a stall exceeds the
         * bounded correction. Silence still means a real underrun. */
        if(s->valid)for(unsigned i=0;i<128;i++)for(unsigned c=0;c<2;c++)block[i*2+c]=(int16_t)((int)s->last[c]*(127-(int)i)/128);
        s->valid=0;s->last[0]=s->last[1]=0;return 0;
    }
    s->level+=(count-s->level)*.1;
    double wanted=1+(s->level-NPS64_AUDIO_TARGET)*(.5/NPS64_AUDIO_TARGET);
    if(wanted<.88)wanted=.88;if(wanted>1.08)wanted=1.08;
    s->tempo+=(wanted-s->tempo)*.1;
    unsigned offset=s->valid?NPS64_AUDIO_SEARCH:0;
    if(s->valid){
        uint64_t best=nps64_audio_match(q,s,rd,offset);
        for(unsigned trial=0;trial<=2*NPS64_AUDIO_SEARCH;trial+=8){uint64_t error=nps64_audio_match(q,s,rd,trial);if(error<best){best=error;offset=trial;}}
        unsigned begin=offset>7?offset-7:0,end=offset+7;if(end>2*NPS64_AUDIO_SEARCH)end=2*NPS64_AUDIO_SEARCH;
        for(unsigned trial=begin;trial<=end;trial++){uint64_t error=nps64_audio_match(q,s,rd,trial);if(error<best){best=error;offset=trial;}}
    }
    for(unsigned i=0;i<NPS64_AUDIO_BLOCK;i++)for(unsigned c=0;c<2;c++){
        int value=nps64_audio_at(q,rd,offset+i,c);
        if(s->valid)value=((int)s->tail[i*2+c]*(NPS64_AUDIO_BLOCK-(int)i)+value*(int)i)/NPS64_AUDIO_BLOCK;
        else if(i<128)value=value*(int)i/128;
        block[i*2+c]=(int16_t)value;s->tail[i*2+c]=nps64_audio_at(q,rd,offset+NPS64_AUDIO_BLOCK+i,c);
    }
    s->last[0]=block[(NPS64_AUDIO_BLOCK-1)*2];s->last[1]=block[(NPS64_AUDIO_BLOCK-1)*2+1];
    s->fraction+=s->tempo*NPS64_AUDIO_BLOCK;unsigned advance=(unsigned)s->fraction;s->fraction-=advance;
    if(!s->valid)advance-=NPS64_AUDIO_SEARCH;
    s->valid=1;__atomic_store_n(&q->read,rd+advance,__ATOMIC_RELEASE);return NPS64_AUDIO_BLOCK;
}
#endif
