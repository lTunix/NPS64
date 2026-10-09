#include "hle_game_material.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static unsigned char atlas[512*512*4],pixels[256*256*4],depth[1];
static HleMaterial material[1];
static HleVertex vertices[3]={
 {{-.8f,-.8f,.5f,1},{1,1,1,1},{0,0}},
 {{.8f,-.8f,.5f,1},{1,1,1,1},{0,0}},
 {{0,.8f,.5f,1},{1,1,1,1},{0,0}}};
int main(void){
 material[0]=(HleMaterial){.rect={0,0,1/512.f,1/512.f},.wrap=3,
     .mux={0xfc127e24,0xfffff9fc,HLE_MATERIAL_VALID,0}};
 atlas[0]=255;atlas[3]=255;
 HleSceneFrame scene={vertices,3,depth,material,atlas};HleRenderer *r=NULL;
 assert(!nps64_hle_renderer_open(&r,&scene));
 assert(nps64_hle_renderer_open(&r,&scene)); /* never replace a live context */
 for(unsigned frame=0;frame<1200;frame++){
     int empty=frame%100==99;
     scene.count=empty?0:3;
     unsigned tx=(frame/25)%8,ty=(frame/200)%8;
     unsigned offset=(ty*64*512+tx*64)*4;
     material[0].rect[0]=tx*64/512.f;material[0].rect[1]=ty*64/512.f;
     /* Repeated texels exercise skipped uploads; every atlas cell is visited. */
     atlas[offset]=(frame/5)&1?0:255;atlas[offset+1]=(frame/5)&1?255:0;atlas[offset+3]=255;
     assert(!nps64_hle_renderer_frame(r,&scene,pixels));
     const unsigned char *p=pixels+(128*256+128)*4;
     assert(p[0]==(empty?0:atlas[offset])&&p[1]==(empty?0:atlas[offset+1])&&p[2]==0&&p[3]==255);
     if(empty)for(unsigned i=0;i<65536;i++)assert(!pixels[i*4]&&!pixels[i*4+1]&&!pixels[i*4+2]);
     if(frame==600){ /* Invalid capacity must not poison a healthy context. */
         scene.count=30003;assert(nps64_hle_renderer_frame(r,&scene,pixels));scene.count=3;
     }
 }
 /* Wide 80x20 texture crosses cell boundaries, including its last column. */
 material[0].rect[0]=128/512.f;material[0].rect[1]=448/512.f;
 material[0].rect[2]=80/512.f;material[0].rect[3]=20/512.f;
 for(unsigned i=0;i<3;i++){vertices[i].uv[0]=.999f;vertices[i].uv[1]=.999f;}
 scene.count=3;
 for(unsigned pass=0;pass<2;pass++){
     for(unsigned y=0;y<20;y++)for(unsigned x=0;x<80;x++){
         unsigned offset=((448+y)*512+128+x)*4;
         atlas[offset]=pass?255:0;atlas[offset+1]=0;atlas[offset+2]=pass?0:255;atlas[offset+3]=255;
     }
     assert(!nps64_hle_renderer_frame(r,&scene,pixels));
     const unsigned char *p=pixels+(128*256+128)*4;
     assert(p[0]==(pass?255:0)&&p[1]==0&&p[2]==(pass?0:255));
 }
 nps64_hle_renderer_release(r);r=NULL;scene.count=3;
 /* Host destruction/recreation, then a new scene. PS4 retains one context. */
 assert(!nps64_hle_renderer_open(&r,&scene));
 assert(!nps64_hle_renderer_frame(r,&scene,pixels));
 nps64_hle_renderer_release(r);
 puts("PASS persistent renderer: 1200 frames, texture updates, empty frames, capacity rejection and host recreation");
}
