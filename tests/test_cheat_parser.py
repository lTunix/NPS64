"""Exercise the exact patched libretro parser under sanitizers, including invalid inputs."""
import sys
from pathlib import Path
sys.path.insert(0,'scripts')
from patch_cheats import FUNCTION
prefix=r'''
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
typedef struct{uint32_t address;int value;}m64p_cheat_code;
static int g_cheat_ctx,adds,enabled,button;static unsigned count;static m64p_cheat_code captured[128];
static int cheat_set_enabled(int*c,const char*n,int e){(void)c;(void)n;enabled=e;return 1;}
static int cheat_add_new(int*c,const char*n,m64p_cheat_code*codes,int ncode){(void)c;(void)n;adds++;count=(unsigned)ncode;memcpy(captured,codes,count*sizeof(*codes));return 1;}
void event_set_gameshark(int active){button=active;}
'''
suffix=r'''
int main(void){
 retro_cheat_set(0,true,"80000010 0001+81000020 ABCD");assert(adds==1&&count==2&&captured[1].value==0xabcd);
 retro_cheat_set(0,false,NULL);assert(!enabled&&adds==1);
 const char*bad[]={"","80000010","800000100 0001","80000010 10000","80000010 ????","80000010 0001 junk","80000010 0001 81000020"};
 for(unsigned i=0;i<sizeof bad/sizeof *bad;i++)retro_cheat_set(1,true,bad[i]);assert(adds==1);
 char code[2000]="";for(unsigned i=0;i<128;i++)strcat(code,i?" 80000010 0001":"80000010 0001");retro_cheat_set(2,true,code);assert(adds==2&&count==128);strcat(code," 80000010 0001");retro_cheat_set(2,true,code);assert(adds==2);
 nps64_cheat_button(1);assert(button==1);nps64_cheat_button(0);assert(!button);
 puts("PASS patched core cheat parser: bounded valid/multiline input, no allocation overflow, invalid/odd/overlong rejection, disable preserves codes and GS button");return 0;
}
'''
Path('build/cheats-33/test-core-parser.c').write_text(prefix+FUNCTION+suffix)
