/* Native core integration fixture: a culled child must not execute its tail. */
#include "rdp_emit_f3d.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static void command(unsigned char *ram,unsigned offset,uint32_t a,uint32_t b){memcpy(ram+offset,&a,4);memcpy(ram+offset+4,&b,4);}
static void check(int legacy,int modern,unsigned first,unsigned end,int visible){
 unsigned char ram[1024]={0},storage[256];GSPState state;RdpFifo fifo;gsp_init(&state);
 state.nps64_culldl_legacy=legacy;
 for(unsigned i=0;i<8;i++)state.vtx[i].clip=GSP_CLIP_NX;
 if(first)for(unsigned i=0;i<first;i++)state.vtx[i].clip=0;
 if(visible)state.vtx[first].clip=0;
 command(ram,64,0xbe000000|(legacy?first*40:first*2),legacy?end*40:(end-1)*2);
 command(ram,72,0xb7000000,0x00020000);command(ram,80,0xb8000000,0);
 f3d_seg_reset();f3d_set_rdram(ram);f3d_set_rdram_size(sizeof(ram));f3d_set_variant_f3dex(modern);
 rdp_fifo_init(&fifo,storage,0,sizeof(storage));f3d_run_dl(&state,&fifo,64,0,0);
 assert(!!(state.geometry_mode&0x00020000)==!!visible);
 f3d_set_rdram(NULL);f3d_set_rdram_size(0);
}
__attribute__((constructor))static void tests(void){
 check(1,1,0,8,0);check(1,1,2,8,0);check(1,1,0,8,1);
 check(0,1,2,8,0);check(0,1,2,8,1);check(1,0,0,8,0);
 puts("PASS core F3D/F3DEX cull dispatch: exact range, nonzero starts, culled tails and visible lists");
 if(getenv("NPS64_CULL_FIXTURE_ONLY"))exit(0);
}
