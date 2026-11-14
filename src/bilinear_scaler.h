/* Centered, separable presentation filter. RDP rendering stays on Vulkan. */
#pragma once
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "software_scaler.h"
enum { NPS64_SMOOTH_MAX_WIDTH=1280 };
typedef struct {
 unsigned index[NPS64_SMOOTH_MAX_WIDTH],weight[NPS64_SMOOTH_MAX_WIDTH];
 uint16_t lanes[NPS64_SMOOTH_MAX_WIDTH][4];
 uint32_t rows[2][NPS64_SMOOTH_MAX_WIDTH];
 unsigned source_width,output_width;
} Nps64BilinearScaler;
static inline void nps64_smooth_position(unsigned x,unsigned source,unsigned output,unsigned *index,unsigned *weight){
 int64_t position=(int64_t)(((uint64_t)(2*x+1)*source*128)/output)-128;
 if(position<0)position=0;
 if(position>(int64_t)(source-1)*256)position=(int64_t)(source-1)*256;
 *index=(unsigned)(position>>8);*weight=(unsigned)position&255;
}
static inline uint32_t nps64_smooth_lerp(uint32_t a,uint32_t b,unsigned f){
 unsigned q=256-f;
 uint32_t rb=(((a&0xff00ff)*q+(b&0xff00ff)*f+0x800080)>>8)&0xff00ff;
 uint32_t g=(((a&0xff00)*q+(b&0xff00)*f+0x8000)>>8)&0xff00;
 return 0xff000000|rb|g;
}
#if defined(__SSE2__)
/* Four adjacent input pixels produce three output pixels at phases
 * 42/256, 128/256 and 213/256. This is exactly the generic centered map
 * for 1280 -> 960 (and other 4:3 reductions); avoid scalar gathers. */
static inline __m128i nps64_smooth_four_to_three(__m128i input,int rgba){
 const __m128i zero=_mm_setzero_si128(),full=_mm_set1_epi16(256),round=_mm_set1_epi16(128);
 __m128i right=_mm_shuffle_epi32(input,_MM_SHUFFLE(3,3,2,1));
 __m128i low_weight=_mm_set_epi16(128,128,128,128,42,42,42,42);
 __m128i high_weight=_mm_set_epi16(0,0,0,0,213,213,213,213);
 __m128i lo=_mm_add_epi16(_mm_mullo_epi16(_mm_unpacklo_epi8(input,zero),_mm_sub_epi16(full,low_weight)),_mm_mullo_epi16(_mm_unpacklo_epi8(right,zero),low_weight));
 __m128i hi=_mm_add_epi16(_mm_mullo_epi16(_mm_unpackhi_epi8(input,zero),_mm_sub_epi16(full,high_weight)),_mm_mullo_epi16(_mm_unpackhi_epi8(right,zero),high_weight));
 lo=_mm_srli_epi16(_mm_add_epi16(lo,round),8);hi=_mm_srli_epi16(_mm_add_epi16(hi,round),8);
 return nps64_rgba_to_argb(_mm_packus_epi16(lo,hi),rgba);
}
#endif
static __attribute__((noinline)) void nps64_smooth_row(Nps64BilinearScaler *s,uint32_t *dst,const unsigned char *src,unsigned width,unsigned output,int rgba){
 unsigned x=0;
#if defined(__SSE2__) && !defined(NPS64_SMOOTH_GENERIC)
 if(width==output){
  for(;x+4<=output;x+=4)_mm_storeu_si128((__m128i*)(dst+x),nps64_rgba_to_argb(_mm_loadu_si128((const __m128i*)(src+x*4)),rgba));
 }else if(width%8==0 && (uint64_t)width*3==(uint64_t)output*4){
  for(unsigned i=0;i<width;i+=8){
   __m128i a=nps64_smooth_four_to_three(_mm_loadu_si128((const __m128i*)(src+i*4)),rgba);
   __m128i b=nps64_smooth_four_to_three(_mm_loadu_si128((const __m128i*)(src+(i+4)*4)),rgba);
   __m128i first=_mm_or_si128(_mm_and_si128(a,_mm_set_epi32(0,-1,-1,-1)),_mm_slli_si128(b,12));
   _mm_storeu_si128((__m128i*)(dst+x),first);
   _mm_storel_epi64((__m128i*)(dst+x+4),_mm_srli_si128(b,4));
   x+=6;
  }
 }
#endif
#if defined(__SSE2__)
 const __m128i zero=_mm_setzero_si128(),full=_mm_set1_epi16(256),round=_mm_set1_epi16(128);
 for(;x+2<=output;x+=2){
  unsigned i=s->index[x],j=i+1<width?i+1:i,k=s->index[x+1],l=k+1<width?k+1:k;
  uint32_t a,b,c,d;memcpy(&a,src+i*4,4);memcpy(&b,src+j*4,4);memcpy(&c,src+k*4,4);memcpy(&d,src+l*4,4);
  __m128i va=_mm_unpacklo_epi8(_mm_set_epi32(0,0,(int)c,(int)a),zero),vb=_mm_unpacklo_epi8(_mm_set_epi32(0,0,(int)d,(int)b),zero);
  __m128i f=_mm_loadu_si128((const __m128i*)s->lanes[x]);
  __m128i value=_mm_add_epi16(_mm_mullo_epi16(va,_mm_sub_epi16(full,f)),_mm_mullo_epi16(vb,f));
  value=_mm_srli_epi16(_mm_add_epi16(value,round),8);
  value=nps64_rgba_to_argb(_mm_packus_epi16(value,zero),rgba);
  _mm_storel_epi64((__m128i*)(dst+x),value);
 }
#endif
 for(;x<output;x++){
  unsigned i=s->index[x],j=i+1<width?i+1:i;uint32_t a,b;
  memcpy(&a,src+i*4,4);memcpy(&b,src+j*4,4);
  uint32_t v=nps64_smooth_lerp(a,b,s->weight[x]);
  if(rgba)v=(v&0xff00ff00)|((v&255)<<16)|((v>>16)&255);
  dst[x]=v;
 }
}
static inline int nps64_scale_bilinear32(Nps64BilinearScaler *s,uint32_t *dst,unsigned stride,const void *pixels,size_t pitch,unsigned width,unsigned height,unsigned output_width,unsigned output_height,int rgba){
 if(!width||!height||!output_width||!output_height||output_width>NPS64_SMOOTH_MAX_WIDTH||stride<output_width||pitch<(size_t)width*4)return 0;
 if(s->source_width!=width||s->output_width!=output_width){
  for(unsigned x=0;x<output_width;x++){
   nps64_smooth_position(x,width,output_width,&s->index[x],&s->weight[x]);
   for(unsigned c=0;c<4;c++)s->lanes[x][c]=(uint16_t)s->weight[x];
  }
  s->source_width=width;s->output_width=output_width;
 }
 unsigned cached[2]={~0u,~0u};
 for(unsigned y=0;y<output_height;y++){
  unsigned i,f;nps64_smooth_position(y,height,output_height,&i,&f);unsigned j=i+1<height?i+1:i;
  unsigned rows[2]={i,j};
  for(unsigned k=0;k<2;k++){
   unsigned row=rows[k],slot=row&1;
   if(cached[slot]!=row){nps64_smooth_row(s,s->rows[slot],(const unsigned char*)pixels+(size_t)row*pitch,width,output_width,rgba);cached[slot]=row;}
  }
  const uint32_t *a=s->rows[i&1],*b=s->rows[j&1];uint32_t *out=dst+(size_t)y*stride;unsigned x=0;
#ifndef NPS64_SMOOTH_GENERIC
  if(!f||i==j){memcpy(out,a,output_width*4);continue;}
#if defined(__SSE2__)
  if(f==128){
   for(;x+4<=output_width;x+=4)_mm_storeu_si128((__m128i*)(out+x),_mm_avg_epu8(_mm_loadu_si128((const __m128i*)(a+x)),_mm_loadu_si128((const __m128i*)(b+x))));
  }
#endif
#endif
#if defined(__SSE2__)
  __m128i zero=_mm_setzero_si128(),wa=_mm_set1_epi16((short)(256-f)),wb=_mm_set1_epi16((short)f),round=_mm_set1_epi16(128);
  for(;x+4<=output_width;x+=4){
   __m128i va=_mm_loadu_si128((const __m128i*)(a+x)),vb=_mm_loadu_si128((const __m128i*)(b+x));
   __m128i lo=_mm_add_epi16(_mm_mullo_epi16(_mm_unpacklo_epi8(va,zero),wa),_mm_mullo_epi16(_mm_unpacklo_epi8(vb,zero),wb));
   __m128i hi=_mm_add_epi16(_mm_mullo_epi16(_mm_unpackhi_epi8(va,zero),wa),_mm_mullo_epi16(_mm_unpackhi_epi8(vb,zero),wb));
   lo=_mm_srli_epi16(_mm_add_epi16(lo,round),8);hi=_mm_srli_epi16(_mm_add_epi16(hi,round),8);
   _mm_storeu_si128((__m128i*)(out+x),_mm_packus_epi16(lo,hi));
  }
#endif
  for(;x<output_width;x++)out[x]=nps64_smooth_lerp(a[x],b[x],f);
 }
 return 1;
}
