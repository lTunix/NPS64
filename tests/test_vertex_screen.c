#include "rdp_emit_frontend.h"
#include "rdp_emit_rsp.h"
#include <assert.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
extern void nps64_reference_bridge_compute_screen(const BridgeVertex*,const BridgeViewport*,int32_t*,int32_t*,int32_t*,int64_t*,int*,int32_t*);
static unsigned rng=1234567;
static unsigned next(void){rng=rng*1664525u+1013904223u;return rng;}
int main(void){
 GSPState s;gsp_init(&s);unsigned checked=0;
 for(unsigned i=0;i<300000;i++){
  BridgeVertex v={0};BridgeViewport vp=s.viewport;int32_t a[5],b[5];int64_t wa,wb;int oa,ob;
  v.cw=(int32_t)next();v.cx=(int32_t)next();v.cy=(int32_t)next();v.cz=(int32_t)next();if(i%20==0)v.cw=0;
  vp.persp_norm=next()&65535;vp.rs_model=i%3==0;vp.rsp_screen_model=i%3!=2;
  for(unsigned k=0;k<4;k++){vp.rs_vs[k]=(int)(next()%1024);vp.rs_vt[k]=(int)(next()%1024);}
  rsp_set_vtx_invw_2rd(i%2);rsp_set_vtx_invw_raw(i%7==0);rsp_set_vtx_z_quant(i%11==0);rsp_set_vtx_y_round(i%13==0);
  nps64_reference_bridge_compute_screen(&v,&vp,a,a+1,a+2,&wa,&oa,a+3);
  bridge_compute_screen(&v,&vp,b,b+1,b+2,&wb,&ob,b+3);
  assert(!memcmp(a,b,4*sizeof(*a))&&wa==wb&&oa==ob);checked++;
 }
 printf("PASS screen oracle: %u exact outputs, standard/RS/fallback models, positive/negative/zero W and rounding profiles\n",checked);
 return 0;
}
