#include "parallel_compute_probe.h"
#include <stdio.h>

static void report(const char *message) {puts(message);fflush(stdout);}
int main(void) {
    ParallelComputeProbe *probe=NULL;
    if(parallel_probe_open(&probe,report))return 1;
    if(parallel_probe_run(probe,PARALLEL_PROBE_CASES,1)!=1 ||
       parallel_probe_run(probe,0,0)!=1 ||
       parallel_probe_run(probe,0,PARALLEL_PROBE_REPEATS+1)!=1)return 1;
    for(unsigned i=0;i<PARALLEL_PROBE_CASES;i++)
        if(parallel_probe_run(probe,i,PARALLEL_PROBE_REPEATS))return 1;
    parallel_probe_close(probe);
    puts("PASS all seven cases: 448 fenced submissions, 896 dispatches; exact output and guard; host only");
    return 0;
}
