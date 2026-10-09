#include "hle_game_material.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static unsigned char atlas[512*512*4],pixels[256*256*4],depth[10000];
static HleVertex vertices[30000];
static HleMaterial materials[10000];
typedef struct {unsigned next,present,stop,fade;} Context;
static int next(unsigned frame,HleSceneFrame *scene,void *opaque){
    Context *c=opaque;assert(frame==c->next++);assert(frame==c->present);
    if(frame==c->stop)return 0;
    memset(vertices,0,sizeof(vertices));memset(materials,0,sizeof(materials));
    float pos[6][2]={{-.75f,-.75f},{.75f,-.75f},{.75f,.75f},{-.75f,-.75f},{.75f,.75f},{-.75f,.75f}};
    for(unsigned i=0;i<6;i++){
        vertices[i].position[0]=pos[i][0];vertices[i].position[1]=pos[i][1];vertices[i].position[3]=1;
        for(unsigned k=0;k<4;k++)vertices[i].color[k]=1;
    }
    unsigned char rgba[4]={frame&1?0:255,frame&1?255:0,0,255};
    if(frame<c->fade)rgba[0]=rgba[1]=0;
    memcpy(atlas+(64*512+128)*4,rgba,4);
    for(unsigned i=0;i<2;i++){
        materials[i].rect[0]=128/512.f;materials[i].rect[1]=64/512.f;
        materials[i].rect[2]=materials[i].rect[3]=1/512.f;
        /* Alternate scissor state as well as triangle count and texels. */
        if(frame&1){materials[i].mux[2]=HLE_SCISSOR;materials[i].scissor[2]=materials[i].scissor[3]=1;materials[i].scissor[0]=.5f;}
    }
    size_t count=frame&1?6:3;
    if(frame==119){
        for(unsigned i=6;i<30000;i++)vertices[i]=vertices[i%6];
        for(unsigned i=2;i<10000;i++)materials[i]=materials[i%2];
        count=30000;
    }
    *scene=(HleSceneFrame){vertices,count,depth,materials,atlas};return 1;
}
static void present(unsigned frame,const unsigned char *p,void *opaque){
    Context *c=opaque;assert(frame==c->present++);
    if(frame<c->fade){for(unsigned i=0;i<65536;i++)assert(!p[i*4]&&!p[i*4+1]&&!p[i*4+2]);return;}
    const unsigned char *inside=p+(96*256+160)*4,*left=p+(96*256+96)*4,*corner=p;
    unsigned char expected[4]={frame&1?0:255,frame&1?255:0,0,255},black[4]={0,0,0,255};
    assert(!memcmp(inside,expected,4));assert(!memcmp(corner,black,4));
    assert(!memcmp(left,frame&1?black:expected,4));
}
int main(void){
    Context c={.stop=120};HleSequence sequence={next,present,&c,120};
    assert(nps64_hle_scene_sequence(&sequence,pixels)==0);assert(c.next==120&&c.present==120);
    c=(Context){.stop=8,.fade=2};sequence.frames=8;
    assert(nps64_hle_scene_sequence(&sequence,pixels)==0);assert(c.present==8);
    c=(Context){.stop=8,.fade=8};
    assert(nps64_hle_scene_sequence(&sequence,pixels)!=0);assert(c.present==8);
    /* A second native session exercises destruction/recreation and cancellation. */
    c=(Context){.stop=7};sequence.frames=64;
    assert(nps64_hle_scene_sequence(&sequence,pixels)==0);assert(c.next==8&&c.present==7);
    sequence.frames=121;assert(nps64_hle_scene_sequence(&sequence,pixels)!=0);
    puts("PASS 120 changing frames: atlas, vertex count, scissor, callback order; cancellation and native resource lifetime");
}
