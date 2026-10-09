#include "software_scaler.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static unsigned char input[4096*4+32];
static uint32_t output[4096*4+32];
int main(void){
    unsigned widths[]={4,8,320,640,1280};
    uint32_t seed=1234567;
    for(unsigned i=0;i<sizeof(input);i++){seed=seed*1664525u+1013904223u;input[i]=(unsigned char)(seed>>24);}
    for(unsigned k=0;k<sizeof(widths)/sizeof(*widths);k++)for(unsigned ratio=0;ratio<4;ratio++)for(unsigned rgba=0;rgba<2;rgba++)for(unsigned alignment=0;alignment<4;alignment++){
        unsigned w=widths[k],dw=ratio? w*(ratio+1):w/2*3;
        memset(output,0x5a,sizeof(output));
        int ok=nps64_scale_row32(output+1,input+alignment,w,dw,rgba);
        if(!ok){assert(w%8&&ratio==0);continue;}
        for(unsigned x=0;x<dw;x++){
            uint32_t c;memcpy(&c,input+alignment+(x*w/dw)*4,4);
            if(rgba)c=(c&0xff00ff00u)|((c&255)<<16)|((c>>16)&255);
            assert(output[x+1]==(c|0xff000000u));
        }
        assert(output[0]==0x5a5a5a5a&&output[dw+1]==0x5a5a5a5a);
    }
    memset(output,0x5a,sizeof(output));assert(!nps64_scale_row32(output,input,319,960,0));assert(output[0]==0x5a5a5a5a);
    puts("PASS SSE2 scaler: exact scalar pixels, RGB/RGBA, 3:2 and 2x/3x/4x, unaligned inputs and bounds");
}
