#include "hle_game_geometry.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static unsigned char pixels[256*256*4];
static HleVertex vertex(float x,float y,float z,float r,float g,float b) {
    HleVertex v={{x,y,z,1},{r,g,b,1},{0,0}};return v;
}
int main(void) {
    HleVertex v[12];unsigned char depth[4]={0,0,1,0};
    v[0]=vertex(-1,-1,0,1,1,1);v[1]=vertex(1,-1,0,1,1,1);v[2]=vertex(1,1,0,1,1,1);
    v[3]=v[0];v[4]=v[2];v[5]=vertex(-1,1,0,1,1,1);
    v[6]=vertex(-.8f,-.8f,.5f,1,0,0);v[7]=vertex(.8f,-.8f,.5f,1,0,0);v[8]=vertex(0,.8f,.5f,1,0,0);
    assert(nps64_hle_scene_probe(v,9,depth,pixels)==0);
    assert(pixels[(128*256+128)*4]==255 && pixels[(128*256+128)*4+1]==0);
    assert(pixels[0]==255 && pixels[1]==255 && pixels[2]==255);
    for(int i=0;i<3;i++){v[9+i]=v[6+i];v[9+i].position[2]=.9f;v[9+i].color[0]=0;v[9+i].color[1]=1;}
    assert(nps64_hle_scene_probe(v,12,depth,pixels)==0);
    assert(pixels[(128*256+128)*4]==0 && pixels[(128*256+128)*4+1]==255);
    assert(nps64_hle_scene_probe(v,6,depth,pixels)!=0);
    puts("PASS depth-disabled background, ordered overlay and uniform snapshot rejection");
    return 0;
}
