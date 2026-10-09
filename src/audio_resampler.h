#ifndef NPS64_AUDIO_RESAMPLER_H
#define NPS64_AUDIO_RESAMPLER_H
#include <stdint.h>
#include <stddef.h>
#include <math.h>
#include <string.h>
#if defined(__SSE2__) && !defined(NPS64_AUDIO_RESAMPLE_SCALAR)
#include <emmintrin.h>
#endif
enum { NPS64_RESAMPLE_TAPS=32,NPS64_RESAMPLE_PHASES=256 };
typedef struct {
    float coefficients[NPS64_RESAMPLE_PHASES][NPS64_RESAMPLE_TAPS];
    float left[NPS64_RESAMPLE_TAPS*2],right[NPS64_RESAMPLE_TAPS*2];
    unsigned head;
    double rate,position;
    int primed;
} Nps64AudioResampler;
typedef void (*Nps64AudioEmit)(void*,int16_t,int16_t);
static inline void nps64_resampler_reset(Nps64AudioResampler *r) {
    r->position=0;r->primed=0;r->head=0;
}
static inline void nps64_resampler_rate(Nps64AudioResampler *r,double rate) {
    if(r->rate==rate)return;
    r->rate=rate;nps64_resampler_reset(r);
    const double pi=3.14159265358979323846;
    double cutoff=rate>48000?48000/rate:1;
    cutoff*=0.94;
    for(unsigned p=0;p<NPS64_RESAMPLE_PHASES;p++) {
        double fraction=(double)p/NPS64_RESAMPLE_PHASES,sum=0;
        for(unsigned j=0;j<NPS64_RESAMPLE_TAPS;j++) {
            double x=(double)j+fraction-15;
            double sinc=fabs(x)<1e-12?cutoff:sin(pi*cutoff*x)/(pi*x);
            double window=0.42+0.5*cos(pi*x/16)+0.08*cos(2*pi*x/16);
            double c=fabs(x)>16?0:sinc*window;
            r->coefficients[p][NPS64_RESAMPLE_TAPS-1-j]=(float)c;sum+=c;
        }
        for(unsigned j=0;j<NPS64_RESAMPLE_TAPS;j++)r->coefficients[p][j]/=(float)sum;
    }
}
static inline int16_t nps64_audio_quantize(float v) {
    if(v>32767)return 32767;if(v<-32768)return -32768;
    return (int16_t)(v>=0?v+0.5f:v-0.5f);
}
/* Causal polyphase FIR, 15 input samples of delay; state spans every batch.
   Tables are rebuilt only when the core changes its DAC rate. */
static inline void nps64_resample(Nps64AudioResampler *r,const int16_t *samples,
                                 size_t frames,double rate,Nps64AudioEmit emit,void *context) {
    if(!samples || !frames || !isfinite(rate) || rate<8000 || rate>192000)return;
    nps64_resampler_rate(r,rate);
    double step=rate/48000;
    for(size_t i=0;i<frames;i++) {
        if(!r->primed) {
            for(unsigned j=0;j<NPS64_RESAMPLE_TAPS*2;j++) {
                r->left[j]=samples[i*2];r->right[j]=samples[i*2+1];
            }
            r->primed=1;
        }
        r->head=(r->head+1)%NPS64_RESAMPLE_TAPS;
        r->left[r->head]=r->left[r->head+NPS64_RESAMPLE_TAPS]=samples[i*2];
        r->right[r->head]=r->right[r->head+NPS64_RESAMPLE_TAPS]=samples[i*2+1];
        while(r->position<1) {
            unsigned p=(unsigned)(r->position*NPS64_RESAMPLE_PHASES);
            float l=0,rr=0;
            const float *left=r->left+r->head+1,*right=r->right+r->head+1;
            const float *coefficients=r->coefficients[p];
#if defined(__SSE2__) && !defined(NPS64_AUDIO_RESAMPLE_SCALAR)
            __m128 vl=_mm_setzero_ps(),vr=_mm_setzero_ps();
            for(unsigned j=0;j<NPS64_RESAMPLE_TAPS;j+=4) {
                __m128 c=_mm_loadu_ps(coefficients+j);
                vl=_mm_add_ps(vl,_mm_mul_ps(_mm_loadu_ps(left+j),c));
                vr=_mm_add_ps(vr,_mm_mul_ps(_mm_loadu_ps(right+j),c));
            }
            float values[4];_mm_storeu_ps(values,vl);l=(values[0]+values[1])+(values[2]+values[3]);
            _mm_storeu_ps(values,vr);rr=(values[0]+values[1])+(values[2]+values[3]);
#else
            for(unsigned j=0;j<NPS64_RESAMPLE_TAPS;j++) {
                l+=left[j]*coefficients[j];rr+=right[j]*coefficients[j];
            }
#endif
            emit(context,nps64_audio_quantize(l),nps64_audio_quantize(rr));
            r->position+=step;
        }
        r->position-=1;
    }
}
#endif
