/* Exact nearest-neighbour XRGB conversion for common N64 output widths. */
#pragma once
#include <stdint.h>
#include <stddef.h>
#if defined(__SSE2__)
#include <emmintrin.h>
static inline __m128i nps64_rgba_to_argb(__m128i value,int rgba){
    if(rgba){
        __m128i keep=_mm_and_si128(value,_mm_set1_epi32((int)0xff00ff00u));
        __m128i red=_mm_slli_epi32(_mm_and_si128(value,_mm_set1_epi32(255)),16);
        __m128i blue=_mm_and_si128(_mm_srli_epi32(value,16),_mm_set1_epi32(255));
        value=_mm_or_si128(keep,_mm_or_si128(red,blue));
    }
    return _mm_or_si128(value,_mm_set1_epi32((int)0xff000000u));
}
#endif
static inline int nps64_scale_row32(uint32_t *dst,const unsigned char *src,unsigned width,unsigned output_width,int rgba){
#if defined(__SSE2__)
    if(width && width%8==0 && (size_t)output_width*2==(size_t)width*3){
        for(unsigned x=0;x<width;x+=8){
            __m128i a=nps64_rgba_to_argb(_mm_loadu_si128((const __m128i *)(src+x*4)),rgba);
            __m128i b=nps64_rgba_to_argb(_mm_loadu_si128((const __m128i *)(src+(x+4)*4)),rgba);
            uint32_t *out=dst+x/2*3;
            _mm_storeu_si128((__m128i *)out,_mm_shuffle_epi32(a,_MM_SHUFFLE(2,1,0,0)));
            __m128 mixed=_mm_shuffle_ps(_mm_castsi128_ps(a),_mm_castsi128_ps(b),_MM_SHUFFLE(0,0,3,2));
            _mm_storeu_si128((__m128i *)(out+4),_mm_castps_si128(mixed));
            _mm_storeu_si128((__m128i *)(out+8),_mm_shuffle_epi32(b,_MM_SHUFFLE(3,2,2,1)));
        }
        return 1;
    }
    if(!width || width%4 || output_width%width)return 0;
    unsigned factor=output_width/width;
    if(factor<2||factor>4)return 0;
    for(unsigned x=0;x<width;x+=4){
        __m128i a=nps64_rgba_to_argb(_mm_loadu_si128((const __m128i *)(src+x*4)),rgba);
        uint32_t *out=dst+x*factor;
        if(factor==2){
            _mm_storeu_si128((__m128i *)out,_mm_unpacklo_epi32(a,a));
            _mm_storeu_si128((__m128i *)(out+4),_mm_unpackhi_epi32(a,a));
        }else if(factor==3){
            _mm_storeu_si128((__m128i *)out,_mm_shuffle_epi32(a,_MM_SHUFFLE(1,0,0,0)));
            _mm_storeu_si128((__m128i *)(out+4),_mm_shuffle_epi32(a,_MM_SHUFFLE(2,2,1,1)));
            _mm_storeu_si128((__m128i *)(out+8),_mm_shuffle_epi32(a,_MM_SHUFFLE(3,3,3,2)));
        }else{
            _mm_storeu_si128((__m128i *)out,_mm_shuffle_epi32(a,0));
            _mm_storeu_si128((__m128i *)(out+4),_mm_shuffle_epi32(a,0x55));
            _mm_storeu_si128((__m128i *)(out+8),_mm_shuffle_epi32(a,0xaa));
            _mm_storeu_si128((__m128i *)(out+12),_mm_shuffle_epi32(a,0xff));
        }
    }
    return 1;
#else
    (void)dst;(void)src;(void)width;(void)output_width;(void)rgba;return 0;
#endif
}
