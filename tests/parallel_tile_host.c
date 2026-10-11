#include "parallel_compute_probe.h"
#include <stdio.h>
#include <stdlib.h>
static void report(const char *message){puts(message);fflush(stdout);}
int main(void){
    ParallelComputeProbe *probe=NULL;
    if(parallel_probe_open(&probe,report))return 1;
    if(parallel_probe_run(probe,PARALLEL_PROBE_CASES,1)!=1 ||
       parallel_probe_run(probe,0,0)!=1 ||
       parallel_probe_run(probe,0,PARALLEL_PROBE_REPEATS+1)!=1)return 1;
    unsigned cases=getenv("NPS64_TILE_RECORD")?1:PARALLEL_PROBE_CASES;
    unsigned repeats=getenv("NPS64_TILE_RECORD")?1:PARALLEL_PROBE_REPEATS;
    const char *chosen=getenv("NPS64_TILE_CASE");
    if(chosen){unsigned i=(unsigned)strtoul(chosen,NULL,10);if(i>=cases||parallel_probe_run(probe,i,repeats))return 1;}
    else for(unsigned i=0;i<cases;i++)if(parallel_probe_run(probe,i,repeats))return 1;
    parallel_probe_close(probe);
    puts(getenv("NPS64_TILE_COMPILE_ONLY")?
         "PASS AMD compilation only; no GPU execution or pixel validation":
         "PASS isolated original tile capture; host only, not PS4 stability");return 0;
}
