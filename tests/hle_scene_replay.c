/* Replay the VI-selected diagnostic scene without running the N64 CPU. */
#include "hle_game_material.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
static void *read_block(const char *prefix,const char *name,size_t *size){
 char path[1024];snprintf(path,sizeof(path),"%s-%s.bin",prefix,name);
 FILE *f=fopen(path,"rb");assert(f);assert(!fseek(f,0,SEEK_END));long n=ftell(f);assert(n>=0);rewind(f);
 void *p=malloc((size_t)n+1);assert(p);assert(fread(p,1,(size_t)n,f)==(size_t)n);fclose(f);*size=(size_t)n;return p;
}
int main(int argc,char **argv){
 if(argc<3){fprintf(stderr,"Usage: replay frame-prefix output.ppm [no-depth|white|affine]\n");return 2;}
 size_t nv,nm,nd,na;HleVertex *v=read_block(argv[1],"vertices",&nv);HleMaterial *m=read_block(argv[1],"materials",&nm);
 unsigned char *d=read_block(argv[1],"depth",&nd),*a=read_block(argv[1],"atlas",&na);
 assert(nv%sizeof(*v)==0&&nm==nv/sizeof(*v)/3*sizeof(*m)&&nd==nv/sizeof(*v)/3&&na==512*512*4);
 for(size_t i=0;i<nd;i++){
  if(argc>3&&!strcmp(argv[3],"no-depth")){m[i].mux[2]&=~(HLE_Z_COMPARE|HLE_Z_WRITE);d[i]=0;}
  if(argc>3&&!strcmp(argv[3],"white")){m[i].mux[0]=0xfcffffff;m[i].mux[1]=0xfffe793c;m[i].mux[2]=HLE_MATERIAL_VALID|(m[i].mux[2]&(HLE_SCISSOR|HLE_Z_COMPARE|HLE_Z_WRITE));m[i].rect[2]=0;for(unsigned j=0;j<3;j++)for(unsigned c=0;c<4;c++)v[i*3+j].color[c]=1;}
  if(argc>3&&!strcmp(argv[3],"normalized")){float scale=0;for(unsigned j=0;j<3;j++)if(v[i*3+j].position[3]>scale)scale=v[i*3+j].position[3];for(unsigned j=0;j<3;j++)for(unsigned c=0;c<4;c++)v[i*3+j].position[c]/=scale;}
  if(argc>3&&!strcmp(argv[3],"affine"))for(unsigned j=0;j<3;j++){HleVertex *p=&v[i*3+j];for(unsigned c=0;c<3;c++)p->position[c]/=p->position[3];p->position[3]=1;}
 }
 HleSceneFrame scene={v,nv/sizeof(*v),d,m,a};HleRenderer *r=NULL;unsigned char pixels[256*256*4];
 assert(!nps64_hle_renderer_open(&r,&scene));assert(!nps64_hle_renderer_frame(r,&scene,pixels));
 FILE *f=fopen(argv[2],"wb");assert(f);fprintf(f,"P6\n256 256\n255\n");for(unsigned i=0;i<65536;i++)assert(fwrite(pixels+i*4,1,3,f)==3);fclose(f);
 nps64_hle_renderer_release(r);free(v);free(m);free(d);free(a);return 0;
}
