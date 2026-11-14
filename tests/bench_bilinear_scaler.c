/* Same workload for generic and optimized presentation, including pixel hashes. */
#ifndef NPS64_BILINEAR_HEADER
#define NPS64_BILINEAR_HEADER "bilinear_scaler.h"
#endif
#include NPS64_BILINEAR_HEADER
#include <stdio.h>
#include <time.h>
static Nps64BilinearScaler state;
static uint32_t pixels[1280*960],out[1280*720];
int main(void){
 const unsigned cases[][4]={{1280,480,960,720},{1280,480,1280,720},{1280,576,960,720},{640,480,960,720}};
 for(unsigned n=0;n<4;n++)for(int rgba=0;rgba<2;rgba++){
  unsigned w=cases[n][0],h=cases[n][1],dw=cases[n][2],dh=cases[n][3];
  for(unsigned i=0;i<w*h;i++)pixels[i]=i*1034147u+0xa73b29u;
  clock_t begin=clock();
  for(unsigned k=0;k<120;k++)nps64_scale_bilinear32(&state,out,dw,pixels,w*4,w,h,dw,dh,rgba);
  double ms=1000.*(clock()-begin)/CLOCKS_PER_SEC/120;
  uint64_t hash=1469598103934665603ULL;for(unsigned i=0;i<dw*dh;i++){hash^=out[i];hash*=1099511628211ULL;}
  printf("%ux%u>%ux%u rgba=%d ms=%.6f hash=%016llx\n",w,h,dw,dh,rgba,ms,(unsigned long long)hash);
 }
}
