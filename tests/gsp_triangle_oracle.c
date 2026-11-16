/* Native real-ROM differential check; never linked into a PS4 package. */
#include "rdp_emit_frontend.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
extern int __real_gsp_triangle(GSPState*,int32_t*,int,int,int,int,int);
extern int nps64_reference_gsp_triangle(GSPState*,int32_t*,int,int,int,int,int);
static unsigned long checked;
static double reference_seconds,optimized_seconds;
static double stamp(void){struct timespec t;clock_gettime(CLOCK_MONOTONIC,&t);return t.tv_sec+t.tv_nsec/1e9;}
int __wrap_gsp_triangle(GSPState*s,int32_t*cmd,int a,int b,int c,int tex,int z){
 int32_t reference[GSP_TRI_CMD_WORDS];int n,expected;double start;
 if(checked&1){start=stamp();n=__real_gsp_triangle(s,cmd,a,b,c,tex,z);optimized_seconds+=stamp()-start;
  start=stamp();expected=nps64_reference_gsp_triangle(s,reference,a,b,c,tex,z);reference_seconds+=stamp()-start;}
 else{start=stamp();expected=nps64_reference_gsp_triangle(s,reference,a,b,c,tex,z);reference_seconds+=stamp()-start;
  start=stamp();n=__real_gsp_triangle(s,cmd,a,b,c,tex,z);optimized_seconds+=stamp()-start;}
 if(n<0||n>GSP_TRI_CMD_WORDS||n!=expected||memcmp(cmd,reference,(size_t)n*sizeof(*cmd))){fprintf(stderr,"FAIL GSP triangle #%lu indices=%d,%d,%d words=%d reference=%d\n",checked,a,b,c,n,expected);abort();}
 checked++;return n;
}
__attribute__((destructor))static void report(void){fprintf(stderr,"GSP_ORACLE checked=%lu mismatches=0 reference_ms=%.3f optimized_ms=%.3f\n",checked,reference_seconds*1000,optimized_seconds*1000);}
