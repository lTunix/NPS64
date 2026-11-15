/* CPU overlays on a Vulkan-owned, WB-cached scanout. GPL-2.0-or-later */
#pragma once
#include <stdint.h>
#include <string.h>
#include <emmintrin.h>
#define NPS64_OVERLAY_WIDTH 1280
#define NPS64_OVERLAY_HEIGHT 720
#define NPS64_OVERLAY_ROW_LINES (NPS64_OVERLAY_WIDTH/16)
typedef struct {uint8_t first[NPS64_OVERLAY_HEIGHT],end[NPS64_OVERLAY_HEIGHT];} Nps64OverlayCache;
static inline void nps64_overlay_reset(Nps64OverlayCache*c){memset(c->first,NPS64_OVERLAY_ROW_LINES,sizeof c->first);memset(c->end,0,sizeof c->end);}
static inline void nps64_overlay_flush_line(void*p){
#ifdef NPS64_TEST_CACHE_FLUSH
 extern void nps64_test_cache_flush_line(void*);nps64_test_cache_flush_line(p);
#else
 _mm_clflush(p);
#endif
}
/* Called BEFORE CPU drawing: invalidate any clean cache lines left from a
 * preceding use. At presentation, write back/invalidate only touched lines.
 * MFENCE completes CLFLUSH before CPU access / the external display engine. */
static inline void nps64_overlay_begin(Nps64OverlayCache*c,void*pixels,int x,int y,int w,int h){
 if(w<=0||h<=0)return;
 int64_t right=(int64_t)x+w,bottom=(int64_t)y+h;
 if(x<0)x=0;if(y<0)y=0;if(right>NPS64_OVERLAY_WIDTH)right=NPS64_OVERLAY_WIDTH;if(bottom>NPS64_OVERLAY_HEIGHT)bottom=NPS64_OVERLAY_HEIGHT;
 if(x>=right||y>=bottom)return;
 unsigned first=(unsigned)x/16,end=((unsigned)right+15)/16;
 for(unsigned row=(unsigned)y;row<(unsigned)bottom;row++){
  if(first<c->first[row])c->first[row]=(uint8_t)first;
  if(end>c->end[row])c->end[row]=(uint8_t)end;
  for(unsigned line=first;line<end;line++)nps64_overlay_flush_line((char*)pixels+(size_t)row*NPS64_OVERLAY_WIDTH*4+line*64);
 }
 _mm_mfence();
}
static inline unsigned nps64_overlay_publish(Nps64OverlayCache*c,void*pixels){
 unsigned count=0;
 for(unsigned row=0;row<NPS64_OVERLAY_HEIGHT;row++)for(unsigned line=c->first[row];line<c->end[row];line++){
  nps64_overlay_flush_line((char*)pixels+(size_t)row*NPS64_OVERLAY_WIDTH*4+line*64);count++;
 }
 if(count)_mm_mfence();return count;
}
