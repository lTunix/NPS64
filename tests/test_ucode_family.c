#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "ucode_family_generated.h"
static void check(const char *banner,int expected){
 unsigned char ram[512]={0};const unsigned data=64;
 for(unsigned i=0;banner[i];i++)ram[(data+i)^3]=(unsigned char)banner[i];
 assert(f3d_ucode_family(ram,sizeof(ram),data,256)==expected);
 assert(f3d_ucode_family(ram,sizeof(ram),data,4)==0);
}
int main(void){
 check("RSP Gfx ucode F3DEX         0.95 Yoshitaka Yasumoto Nintendo.",1);
 check("RSP Gfx ucode F3DLX         0.95 Yoshitaka Yasumoto Nintendo.",1);
 check("RSP Gfx ucode F3DEX         1.21 Yoshitaka Yasumoto Nintendo.",1);
 check("RSP Gfx ucode L3DEX         1.21 Yoshitaka Yasumoto Nintendo.",2);
 check("RSP Gfx ucode F3DLX.Rej   xbus 2.08 Yoshitaka Yasumoto 1999 Nintendo.",0);
 check("RSP Gfx ucode F3DEX2     fifo 2.08",0);
 check("RSP Gfx ucode F3DEX.NoN  fifo 2.08",0);
 check("RSP Gfx ucode L3DEX      xbus 2.08",0);
 check("RSP Gfx ucode S2DEX      xbus 2.08",0);
 assert(f3d_ucode_family(NULL,512,64,256)==0);
 puts("PASS microcode family: GBI 1 0.95/1.21, line family, GBI 2 fifo/xbus and bounded banners");
 return 0;
}
