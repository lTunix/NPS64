#define _POSIX_C_SOURCE 200809L
#include "cheats.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
static unsigned reset_calls,set_calls,disable_calls;
static void reset(void){reset_calls++;}
static void set(unsigned i,bool on,const char*code){(void)i;set_calls++;if(on)assert(nps64_cheat_code_valid(code));else {assert(!code);disable_calls++;}}
static void header(const char*path,unsigned order,unsigned country){unsigned char b[64]={0x80,0x37,0x12,0x40};b[16]=0x12;b[20]=0x34;b[62]=(unsigned char)country;if(order==1)for(unsigned i=0;i<64;i+=2){unsigned char v=b[i];b[i]=b[i+1];b[i+1]=v;}if(order==2)for(unsigned i=0;i<64;i+=4){unsigned char a=b[i],v=b[i+1];b[i]=b[i+3];b[i+1]=b[i+2];b[i+2]=v;b[i+3]=a;}assert(file_write_atomic(path,b,64));}
int main(void){
 char dir[]="/tmp/nps-cheats-XXXXXX";assert(mkdtemp(dir));char db[1024],rom[1024],cfg[1024],bad[1024];snprintf(db,sizeof db,"%s/library.db",dir);snprintf(rom,sizeof rom,"%s/renamed-game.z64",dir);snprintf(cfg,sizeof cfg,"%s/selections",dir);snprintf(bad,sizeof bad,"%s/missing/dir",dir);
 FILE*f=fopen(db,"wb");assert(f);fputs("NPS64CHEATS1\nR|12000000-34000000-45|Fixture\n",f);
 for(unsigned i=0;i<36;i++)fprintf(f,"C|%016x|%016x|Cheat %u|80000010 %04X\n",i+1,i<2?1:i,i,i);
 fclose(f);Game game={0};strcpy(game.path,rom);strcpy(game.key,"rom-0000000000000001");Nps64Cheats c={0},loaded={0};
 for(unsigned order=0;order<3;order++){header(rom,order,0x45);assert(nps64_cheats_load(&c,&game,db,cfg));assert(c.count==36&&!nps64_cheats_enabled(&c));}
 assert(nps64_cheats_toggle(&c,0,cfg));assert(c.items[0].enabled);assert(nps64_cheats_toggle(&c,1,cfg));assert(!c.items[0].enabled&&c.items[1].enabled);
 nps64_cheats_apply(&c,true,reset,set);assert(reset_calls==1&&set_calls==1);nps64_cheats_apply(&c,false,reset,set);assert(set_calls==1);
 assert(nps64_cheats_toggle(&c,1,cfg));nps64_cheats_apply(&c,false,reset,set);assert(disable_calls==1&&set_calls==2);
 assert(nps64_cheats_toggle(&c,0,cfg));assert(nps64_cheats_load(&loaded,&game,db,cfg));assert(loaded.items[0].enabled&&nps64_cheats_enabled(&loaded)==1);
 assert(!nps64_cheats_toggle(&c,0,bad)&&c.items[0].enabled);
 for(unsigned i=2;i<33;i++)assert(nps64_cheats_toggle(&c,i,cfg));assert(nps64_cheats_enabled(&c)==32);assert(!nps64_cheats_toggle(&c,34,cfg));assert(nps64_cheats_enabled(&c)==32);
 assert(nps64_cheats_disable_all(&c,cfg));assert(!nps64_cheats_enabled(&c));
 strcpy(game.key,"rom-0000000000000002");assert(nps64_cheats_load(&loaded,&game,db,cfg));assert(!nps64_cheats_enabled(&loaded));
 header(rom,0,0x50);assert(nps64_cheats_load(&loaded,&game,db,cfg)&&!loaded.count);
 strcpy(game.key,"../bad");assert(!nps64_cheats_load(&loaded,&game,db,cfg));
 const char*invalid[]={"","80000010","8000001 0001","80000010 00001","80000010 ????","80000010 0001 ","80000010 0001 garbage"};for(unsigned i=0;i<sizeof invalid/sizeof *invalid;i++)assert(!nps64_cheat_code_valid(invalid[i]));
 char too_many[2000]="";for(unsigned i=0;i<129;i++)strcat(too_many,i?" 80000010 0001":"80000010 0001");assert(!nps64_cheat_code_valid(too_many));
 nps64_cheats_free(&c);nps64_cheats_free(&loaded);
 printf("PASS cheats: exact CRC/country, three ROM byte orders, opt-in defaults, variant exclusion, persisted selection, per-ROM isolation, rollback, 32-active limit and malformed codes\n");return 0;
}
