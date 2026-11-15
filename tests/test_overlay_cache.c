/* Simulated cached CPU view and display-engine RAM, exercising actual helpers. */
#define _POSIX_C_SOURCE 200809L
#define NPS64_TEST_CACHE_FLUSH
#include "overlay_cache.h"
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
static unsigned char *cpu,*display;static bool publishing;static unsigned calls;
void nps64_test_cache_flush_line(void*address){
 uintptr_t offset=(uintptr_t)address-(uintptr_t)cpu;assert(!(offset&63)&&offset<1280*720*4);
 if(publishing)memcpy(display+offset,cpu+offset,64);else memcpy(cpu+offset,display+offset,64);calls++;
}
static void frame(unsigned generation,int x,int y,int w,int h){
 Nps64OverlayCache cache;nps64_overlay_reset(&cache);memset(display,(int)generation,1280*720*4);memset(cpu,0xad,1280*720*4);
 publishing=false;calls=0;nps64_overlay_begin(&cache,cpu,x,y,w,h);
 unsigned before=calls;
 for(int row=y;row<y+h;row++)for(int col=x;col<x+w;col++)((uint32_t*)cpu)[row*1280+col]=0xff123456;
 publishing=true;unsigned flushed=nps64_overlay_publish(&cache,cpu);assert(flushed==before&&calls==2*before);
 for(unsigned row=0;row<720;row++)for(unsigned col=0;col<1280;col++){
  uint32_t value=((uint32_t*)display)[row*1280+col];bool inside=row>=(unsigned)y&&row<(unsigned)(y+h)&&col>=(unsigned)x&&col<(unsigned)(x+w);
  assert(value==(inside?0xff123456:generation*0x01010101u));
 }
 if(w==1280&&h==44)assert(flushed==3520&&flushed<57600/10);
}
int main(void){
 assert(!posix_memalign((void**)&cpu,256,1280*720*4));assert(!posix_memalign((void**)&display,256,1280*720*4));
 for(unsigned i=0;i<12;i++){frame(i+1,0,12,1280,44);frame(i+1,282,80,716,568);}
 Nps64OverlayCache cache;nps64_overlay_reset(&cache);calls=0;publishing=false;
 nps64_overlay_begin(&cache,cpu,-100,-100,50,50);nps64_overlay_begin(&cache,cpu,1280,720,5,5);assert(!calls);
 nps64_overlay_begin(&cache,cpu,-3,0,7,1);assert(calls==1);
 publishing=true;assert(nps64_overlay_publish(&cache,cpu)==1);nps64_overlay_reset(&cache);assert(nps64_overlay_publish(&cache,cpu)==0);
 free(cpu);free(display);puts("PASS overlay cache: stale CPU view refreshed, HUD/pause published, game pixels/partial-line edges preserved, clipping and bounded HUD flush");return 0;
}
