/* Canvas and scanout are distinct allocations. Never read scanout memory. */
#pragma once
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#if defined(__SSE2__)
#include <emmintrin.h>
#endif
static inline void nps64_scanout_copy(void *destination,const void *source,size_t size){
#if defined(__SSE2__)
    if(!((uintptr_t)destination&15)){
        unsigned char *dst=(unsigned char *)destination;
        const unsigned char *src=(const unsigned char *)source;
        size_t i=0;
        for(;size-i>=64;i+=64){
            __m128i a=_mm_loadu_si128((const __m128i *)(src+i));
            __m128i b=_mm_loadu_si128((const __m128i *)(src+i+16));
            __m128i c=_mm_loadu_si128((const __m128i *)(src+i+32));
            __m128i d=_mm_loadu_si128((const __m128i *)(src+i+48));
            _mm_stream_si128((__m128i *)(dst+i),a);
            _mm_stream_si128((__m128i *)(dst+i+16),b);
            _mm_stream_si128((__m128i *)(dst+i+32),c);
            _mm_stream_si128((__m128i *)(dst+i+48),d);
        }
        if(i<size)memcpy(dst+i,src+i,size-i);
        _mm_sfence();
        return;
    }
#endif
    memcpy(destination,source,size);
#if defined(__SSE2__)
    _mm_sfence();
#endif
}
