#include "hle_game_material.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static unsigned char atlas[512*512*4],pixels[256*256*4];
static HleVertex vertices[6]={
 {{-.8f,-.8f,.5f,1},{.5f,.25f,1,.5f},{0,0}},{{.8f,-.8f,.5f,1},{.5f,.25f,1,.5f},{0,0}},{{0,.8f,.5f,1},{.5f,.25f,1,.5f},{0,0}}};
static HleMaterial m[2];
static unsigned char depth[2]={0,0};
static void center(unsigned r,unsigned g,unsigned b,unsigned a){
 unsigned char *p=pixels+(128*256+128)*4;
 assert(abs((int)p[0]-(int)r)<=1&&abs((int)p[1]-(int)g)<=1&&abs((int)p[2]-(int)b)<=1&&abs((int)p[3]-(int)a)<=1);
}
static void draw(void){assert(nps64_hle_textured_scene_probe(vertices,3,depth,m,atlas,pixels)==0);}
int main(void){
 atlas[0]=128;atlas[1]=64;atlas[2]=32;atlas[3]=255;
 m[0]=(HleMaterial){.rect={0,0,1/512.f,1/512.f},.wrap=3,.prim={.2f,.4f,.6f,.5f},.env={.4f,.2f,.1f,1}};
 unsigned needed;
 /* Exact mux from the user's PS4 Mario 64 log: lit SHADE must survive even
  * a black texture. A white texture selects PRIM. Test nonwhite PRIM too. */
 m[0].mux[0]=0xfc30b261;m[0].mux[1]=0x44664924;m[0].mux[2]=HLE_MATERIAL_VALID;
 assert(nps64_hle_material_decode(m[0].mux[0],m[0].mux[1],0,&needed)&&needed);
 draw();center(89,73,242,128);
 memset(atlas,0,4);draw();center(128,64,255,128);
 memset(atlas,255,4);draw();center(51,102,153,128);
 atlas[0]=128;atlas[1]=64;atlas[2]=32;atlas[3]=255;
 /* Exact equations from castle, text/HUD and normalized COPY/FILL. */
 const uint32_t codes[][2]={
 {0xfcffffff,0xfffe7b3d},{0xfc147e28,0x44fe7b3d},
 {0xfcffffff,0xfffcfa7d},{0xfcffffff,0xfffcf279},
 {0xfcffffff,0xfffcf87c},{0xfc127e24,0xfffff3f9},
 {31,0x001c0041},{31,0x001c00c3}};
 const unsigned expected[][4]={
 {128,64,255,255},{128,64,32,255},{128,64,32,255},{128,64,32,255},
 {128,64,32,128},{64,16,32,255},{128,64,32,255},{51,102,153,128}};
 m[0].mux[2]=HLE_MATERIAL_VALID;
 for(unsigned i=0;i<8;i++){
     m[0].mux[0]=codes[i][0];m[0].mux[1]=codes[i][1];
     assert(nps64_hle_material_decode(codes[i][0],codes[i][1],0,&needed));
     draw();center(expected[i][0],expected[i][1],expected[i][2],expected[i][3]);
 }
 /* cycle 1 RGB=(TEXEL0-ZERO)*SHADE+ZERO; A=SHADE. */
 m[0].mux[0]=(1u<<5)|4u;m[0].mux[1]=(8u<<24)|(7u<<6)|(7u<<18)|4u;
 assert(nps64_hle_material_decode(m[0].mux[0],m[0].mux[1],0,&needed)&&needed);
 m[0].mux[2]=HLE_MATERIAL_VALID;draw();center(64,16,32,128);
 /* Exact castle material and untextured SHADE from the two-run PS4 log. */
 m[0].mux[0]=0xfc127e24;m[0].mux[1]=0xfffff9fc;
 assert(nps64_hle_material_decode(m[0].mux[0],m[0].mux[1],0,&needed)&&needed);
 draw();center(64,16,32,128);
 m[0].mux[0]=0xfcffffff;m[0].mux[1]=0xfffe793c;
 assert(nps64_hle_material_decode(m[0].mux[0],m[0].mux[1],0,&needed)&&!needed);
 draw();center(128,64,255,128);
 /* Direct primitive color, alpha primitive. */
 m[0].mux[0]=31;m[0].mux[1]=(3u<<6)|(7u<<18)|3u;
 assert(nps64_hle_material_decode(m[0].mux[0],m[0].mux[1],0,&needed)&&!needed);
 draw();center(51,102,153,128);
 /* Two cycles: cycle 0=PRIM, cycle 1=COMBINED*ENV. */
 m[0].mux[0]=(31u<<15)|(7u<<9)|5u;
 m[0].mux[1]=(3u<<15)|(7u<<9)|(8u<<24)|(7u<<6)|(7u<<18)|6u;
 m[0].mux[2]=HLE_MATERIAL_VALID|HLE_TWO_CYCLE;
 assert(nps64_hle_material_decode(m[0].mux[0],m[0].mux[1],1,&needed));
 draw();center(20,20,15,255);
 /* Alpha blending over black. */
 m[0].mux[0]=31;m[0].mux[1]=(3u<<6)|(7u<<18)|3u;m[0].mux[2]=HLE_MATERIAL_VALID|HLE_BLEND;
 draw();center(26,51,77,255);
 /* Rejected foreground must leave a red background and its depth intact. */
 memcpy(vertices+3,vertices,3*sizeof(HleVertex));m[1]=m[0];
 m[0].prim[0]=1;m[0].prim[1]=0;m[0].prim[2]=0;m[0].prim[3]=1;m[0].mux[2]=HLE_MATERIAL_VALID|HLE_Z_COMPARE|HLE_Z_WRITE;
 m[1].mux[2]=HLE_MATERIAL_VALID|HLE_ALPHA_TEST|HLE_Z_COMPARE|HLE_Z_WRITE;m[1].mux[3]=200;
 for(unsigned i=3;i<6;i++)vertices[i].position[2]=.2f;
 assert(nps64_hle_textured_scene_probe(vertices,6,depth,m,atlas,pixels)==0);center(255,0,0,255);
 /* MODULATERGBA particle: texture alpha must punch holes and preserve Z.
  * The RGB-only castle equation intentionally has different alpha. */
 m[1].mux[0]=0xfc121824;m[1].mux[1]=0xff33ffff;m[1].mux[3]=128;
 assert(nps64_hle_material_decode(m[1].mux[0],m[1].mux[1],0,&needed)&&needed);
 for(unsigned i=3;i<6;i++)vertices[i].color[3]=1;
 atlas[3]=0;
 assert(nps64_hle_textured_scene_probe(vertices,6,depth,m,atlas,pixels)==0);center(255,0,0,255);
 /* Coverage from texture alpha must work even with alpha compare disabled. */
 m[1].mux[2]=HLE_MATERIAL_VALID|HLE_ALPHA_COVERAGE|HLE_Z_COMPARE|HLE_Z_WRITE;
 assert(nps64_hle_textured_scene_probe(vertices,6,depth,m,atlas,pixels)==0);center(255,0,0,255);
 atlas[3]=255;
 assert(nps64_hle_textured_scene_probe(vertices,6,depth,m,atlas,pixels)==0);center(64,16,32,255);
 atlas[3]=128;
 assert(nps64_hle_textured_scene_probe(vertices,6,depth,m,atlas,pixels)==0);center(159,8,16,255);
 atlas[3]=255;
 for(unsigned i=3;i<6;i++)vertices[i].color[3]=.5f;
 m[1]=m[0];
 /* Compare without write: a later, farther triangle still passes. */
 for(unsigned i=0;i<3;i++)vertices[i].position[2]=.2f;
 for(unsigned i=3;i<6;i++)vertices[i].position[2]=.7f;
 m[0].prim[0]=0;m[0].prim[1]=1;m[0].mux[2]=HLE_MATERIAL_VALID|HLE_Z_COMPARE;
 m[1].prim[0]=1;m[1].prim[1]=0;m[1].prim[2]=0;m[1].prim[3]=1;m[1].mux[2]=HLE_MATERIAL_VALID|HLE_Z_COMPARE|HLE_Z_WRITE;
 assert(nps64_hle_textured_scene_probe(vertices,6,depth,m,atlas,pixels)==0);center(255,0,0,255);
 m[0].mux[2]|=HLE_Z_WRITE;
 assert(nps64_hle_textured_scene_probe(vertices,6,depth,m,atlas,pixels)==0);center(0,255,0,255);
 /* Clip the right half of the foreground. */
 m[0].mux[2]=HLE_MATERIAL_VALID|HLE_SCISSOR;m[0].scissor[0]=.5f;m[0].scissor[1]=0;m[0].scissor[2]=1;m[0].scissor[3]=1;
 draw();assert(pixels[(128*256+120)*4+1]==0);assert(pixels[(128*256+140)*4+1]==255);
 /* SHADE is affine in screen space even when UV uses perspective. */
 m[0].mux[0]=31;m[0].mux[1]=(4u<<6)|(7u<<18)|6u;m[0].mux[2]=HLE_MATERIAL_VALID;
 for(unsigned i=0;i<3;i++){
     float w=(float)(1u<<i);for(unsigned j=0;j<3;j++)vertices[i].position[j]*=w;vertices[i].position[3]=w;
     vertices[i].color[0]=i==0;vertices[i].color[1]=i==1;vertices[i].color[2]=i==2;vertices[i].color[3]=1;
 }
 draw();center(63,64,128,255);
 /* Unsupported TEXEL1 and noise must not be reported as implemented. */
 assert(!nps64_hle_material_decode((2u<<5)|4u,(8u<<24)|(7u<<6)|(7u<<18)|6u,0,&needed));
 assert(!nps64_hle_material_decode((7u<<5)|4u,(8u<<24)|(7u<<6)|(7u<<18)|6u,0,&needed));
 puts("PASS texture*shade, primitive/environment, two cycles, alpha blend/discard and unsupported selectors");
}
