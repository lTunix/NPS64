/* Submission cadence, including previous pacing/I/O; not display timestamps. */
#pragma once
#include <stdint.h>
#include <string.h>
#include <math.h>
typedef struct {
 uint64_t frames,slow_frames,episodes,over_1_5x,over_2x,over50,over100;
 uint64_t active_frames,longest_frames;
 double observed_ms,slow_ms,excess_ms,max_ms,active_ms,longest_ms,active_excess_ms,active_max_ms;
} Nps64CadenceStats;
static inline void nps64_cadence_reset(Nps64CadenceStats *s){memset(s,0,sizeof(*s));}
static inline void nps64_cadence_add(Nps64CadenceStats *s,double interval,double period){
 if(!isfinite(interval)||!isfinite(period)||interval<0||period<=0)return;
 double ms=interval*1000,budget=period*1000;
 s->frames++;s->observed_ms+=ms;if(ms>s->max_ms)s->max_ms=ms;
 if(ms+1e-9>=budget*1.5)s->over_1_5x++;
 if(ms+1e-9>=budget*2)s->over_2x++;
 if(ms+1e-9>=50)s->over50++;
 if(ms+1e-9>=100)s->over100++;
 /* Ignore sub-millisecond clock/display mismatch instead of classifying
  * a steady 59.94 Hz output as a continuous 60.02 Hz performance drop. */
 if(ms>budget+.5){
  if(!s->active_frames)s->episodes++;
  s->slow_frames++;s->slow_ms+=ms;s->excess_ms+=ms-budget;
  s->active_frames++;s->active_ms+=ms;s->active_excess_ms+=ms-budget;
  if(ms>s->active_max_ms)s->active_max_ms=ms;
  if(s->active_ms>s->longest_ms){s->longest_ms=s->active_ms;s->longest_frames=s->active_frames;}
 }else{s->active_frames=0;s->active_ms=s->active_excess_ms=s->active_max_ms=0;}
}
