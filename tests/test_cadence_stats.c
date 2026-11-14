#include "cadence_stats.h"
#include <assert.h>
#include <stdio.h>
int main(void){
 Nps64CadenceStats s={0};double budget=1./60;
 for(unsigned i=0;i<600;i++)nps64_cadence_add(&s,1./59.94,budget);
 assert(s.frames==600&&s.slow_frames==0&&s.episodes==0);
 nps64_cadence_reset(&s);
 for(unsigned i=0;i<50;i++)nps64_cadence_add(&s,.02,budget);
 assert(s.episodes==1&&s.slow_frames==50&&s.active_frames==50&&fabs(s.longest_ms-1000)<1e-6);
 /* Reporting is cumulative; a run continues across reporting intervals. */
 Nps64CadenceStats reported=s;(void)reported;
 nps64_cadence_add(&s,.02,budget);assert(s.episodes==1&&s.longest_frames==51&&fabs(s.active_ms-1020)<1e-6);
 nps64_cadence_add(&s,budget,budget);assert(!s.active_frames&&s.longest_frames==51);
 nps64_cadence_add(&s,.05,budget);nps64_cadence_add(&s,.12,budget);
 assert(s.episodes==2&&s.over_1_5x==2&&s.over_2x==2&&s.over50==2&&s.over100==1);
 assert(s.max_ms==120&&s.active_max_ms==120&&s.active_frames==2&&fabs(s.active_ms-170)<1e-6);
 assert(fabs(s.active_excess_ms-(170-2*1000./60))<1e-6);
 assert(fabs(s.excess_ms-(51*(20-1000./60)+(50-1000./60)+(120-1000./60)))<1e-6);
 uint64_t count=s.frames;nps64_cadence_add(&s,NAN,budget);nps64_cadence_add(&s,INFINITY,budget);nps64_cadence_add(&s,-1,budget);nps64_cadence_add(&s,.02,0);assert(s.frames==count);
 nps64_cadence_reset(&s);nps64_cadence_add(&s,.0201,1./50);assert(s.slow_frames==0);
 nps64_cadence_add(&s,.031,1./50);assert(s.slow_frames==1&&s.over_1_5x==1&&s.over_2x==0);
 puts("PASS cadence: steady clocks, sustained slow runs, cross-report duration, isolated/long stalls, excess time, PAL budget, reset and invalid samples");
}
