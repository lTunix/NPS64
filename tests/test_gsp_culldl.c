#include "gsp_culldl.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static void banner(unsigned char *ram,const char *s){memset(ram,0,256);for(size_t i=0;i<strlen(s);i++)ram[(16+i)^3]=(unsigned char)s[i];}
int main(void){
 int a,b;assert(nps64_culldl_range(0xbe000000,320,1,80,&a,&b)&&a==0&&b==7);
 assert(nps64_culldl_range(0xbe000050,320,1,80,&a,&b)&&a==2&&b==7);
 assert(nps64_culldl_range(0xbe000004,14,0,80,&a,&b)&&a==2&&b==7);
 assert(!nps64_culldl_range(0xbe000000,0,1,80,&a,&b));
 assert(!nps64_culldl_range(0xbe000014,14,0,80,&a,&b));
 assert(!nps64_culldl_range(0xbe000000,0xffff,0,80,&a,&b));
 unsigned char ram[256];banner(ram,"RSP Gfx ucode F3DEX         0.95 Yoshitaka Yasumoto Nintendo.");assert(nps64_f3dex095(ram,sizeof(ram),16));
 banner(ram,"RSP Gfx ucode F3DLX         0.95 Yoshitaka Yasumoto Nintendo.");assert(nps64_f3dex095(ram,sizeof(ram),16));
 banner(ram,"RSP Gfx ucode F3DEX         1.21 Nintendo.");assert(!nps64_f3dex095(ram,sizeof(ram),16));
 banner(ram,"RSP Gfx ucode F3DEX2        2.08 Nintendo.");assert(!nps64_f3dex095(ram,sizeof(ram),16));
 assert(!nps64_f3dex095(ram,17,16));assert(!nps64_f3dex095(ram,sizeof(ram),256));
 puts("PASS GBI1 cull ranges: eight-vertex 0.95 command, nonzero starts, modern indices, malformed ranges and bounded microcode detection");
}
