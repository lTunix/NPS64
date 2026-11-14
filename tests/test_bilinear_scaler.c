#include "bilinear_scaler.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
static Nps64BilinearScaler scaler;
static uint32_t source[640*480],output[1280*720];
static double coordinate(unsigned x,unsigned input,unsigned out){double p=((double)x+.5)*input/out-.5;return p<0?0:p>input-1?input-1:p;}
static uint32_t reference(unsigned x,unsigned y,unsigned w,unsigned h,unsigned dw,unsigned dh,int rgba){
 double sx=coordinate(x,w,dw),sy=coordinate(y,h,dh);unsigned i=(unsigned)sx,j=(unsigned)sy,ii=i+1<w?i+1:i,jj=j+1<h?j+1:j;double fx=sx-i,fy=sy-j;uint32_t v=0xff000000;
 for(unsigned c=0;c<3;c++){
  unsigned shift=c*8;double a=(source[j*w+i]>>shift)&255,b=(source[j*w+ii]>>shift)&255,d=(source[jj*w+i]>>shift)&255,e=(source[jj*w+ii]>>shift)&255;
  unsigned value=(unsigned)lround((a*(1-fx)+b*fx)*(1-fy)+(d*(1-fx)+e*fx)*fy);v|=value<<shift;
 }
 if(rgba)v=(v&0xff00ff00)|((v&255)<<16)|((v>>16)&255);
 return v;
}
int main(void){
 const unsigned shapes[][4]={{1,1,1,1},{1,3,9,11},{3,1,11,9},{2,2,7,5},{17,11,37,29},{320,240,960,720},{640,480,960,720},{640,480,1280,720}};
 for(unsigned t=0;t<sizeof(shapes)/sizeof(shapes[0]);t++)for(int rgba=0;rgba<2;rgba++){
  unsigned w=shapes[t][0],h=shapes[t][1],dw=shapes[t][2],dh=shapes[t][3];
  for(unsigned i=0;i<w*h;i++)source[i]=(i*1634147u+92837u)^0x37ad8941;
  memset(output,0x5a,sizeof(output));
  assert(nps64_scale_bilinear32(&scaler,output,dw,source,w*4,w,h,dw,dh,rgba));
  for(unsigned y=0;y<dh;y++)for(unsigned x=0;x<dw;x++){
   uint32_t a=output[y*dw+x],b=reference(x,y,w,h,dw,dh,rgba);assert(a>>24==255);
   for(unsigned c=0;c<3;c++)assert(abs((int)((a>>(c*8))&255)-(int)((b>>(c*8))&255))<=2);
  }
 }
 source[0]=0x00112233;assert(nps64_scale_bilinear32(&scaler,output,16,source,4,1,1,13,11,0));
 for(unsigned y=0;y<11;y++)for(unsigned x=0;x<13;x++)assert(output[y*16+x]==0xff112233);
 assert(!nps64_scale_bilinear32(&scaler,output,1,source,4,0,1,1,1,0));
 assert(!nps64_scale_bilinear32(&scaler,output,1,source,4,1,1,1281,1,0));
 clock_t start=clock();for(unsigned i=0;i<120;i++)assert(nps64_scale_bilinear32(&scaler,output,1280,source,640*4,640,480,960,720,1));
 printf("PASS bilinear: independent reference <=2 LSB, dimensions, RGBA, edges, constant, stride, guards; native cost %.3f ms/frame\n",1000.*(clock()-start)/CLOCKS_PER_SEC/120);
}
