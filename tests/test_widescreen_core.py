"""Exercise actual patched VI application/restoration with both projection profiles."""
from pathlib import Path
s=Path('build/gpu-game-test/core/mupen64plus-core/src/main/cheat.c').read_text()
definitions=s[s.index('typedef struct cheat_code'):s.index('/* private functions */')]
helpers=s[s.index('static uint16_t read_address_16bit'):s.index('static cheat_t *find_or_create_cheat')]
apply=s[s.index('void cheat_apply_cheats('):s.index('void cheat_delete_all(')]
prefix=r'''
#include "widescreen.h"
#include <stdlib.h>
#include <assert.h>
#include "main/list.h"
#define S8 3
#define S16 2
#define CHEAT_CODE_MAGIC_VALUE UINT32_C(0xdead0000)
#define ENTRY_BOOT 0
#define ENTRY_VI 1
struct ram {void *dram;size_t dram_size;};
struct r4300_core {struct ram *rdram;};
struct cheat_ctx {struct list_head active_cheats;};
static unsigned invalidates;
static void invalidate_r4300_cached_code(struct r4300_core*c,uint32_t a,unsigned n){(void)c;(void)a;(void)n;invalidates++;}
static int event_gameshark_active(void){return 0;}
'''
suffix=r'''
int main(void){
 struct ram ram={calloc(1,8*1024*1024),8*1024*1024};assert(ram.dram);struct r4300_core cpu={&ram};
 for(unsigned p=0;p<2;p++){
  memset(ram.dram,0,ram.dram_size);struct cheat_ctx ctx;INIT_LIST_HEAD(&ctx.active_cheats);
  cheat_t wide={.name="cheat_4096",.enabled=1};INIT_LIST_HEAD(&wide.cheat_codes);list_add_tail(&wide.list,&ctx.active_cheats);
  cheat_code_t codes[16]={0};uint16_t original[16]={0};unsigned n=0;const char*text=nps64_wide_profiles[p].code;
  while(*text){unsigned a,v;int used=0;assert(sscanf(text,"%x %x %n",&a,&v,&used)==2&&used>0&&n<16);text+=used;codes[n].address=a;codes[n].value=v;codes[n].old_value=CHEAT_CODE_MAGIC_VALUE;list_add_tail(&codes[n].list,&wide.cheat_codes);n++;}
  /* A selected profile must not capture uninitialized zero memory at startup. */
  cheat_apply_cheats(&ctx,&cpu,ENTRY_VI);
  for(unsigned i=0;i<n;i++)assert(codes[i].old_value==CHEAT_CODE_MAGIC_VALUE);
  for(unsigned i=0;i<n;i++)if((codes[i].address>>24)==0x81)update_address_16bit(&cpu,codes[i].address,(uint16_t)(0x3faa+i));
  for(unsigned i=0;i<n;i++)if((codes[i].address>>24)==0xd1)update_address_16bit(&cpu,codes[i].address,codes[i].value);
  for(unsigned i=0;i<n;i++)original[i]=read_address_16bit(&cpu,codes[i].address);
  for(unsigned repeat=0;repeat<20;repeat++){
   wide.enabled=1;cheat_apply_cheats(&ctx,&cpu,ENTRY_VI);
   for(unsigned i=0;i<n;i++)if((codes[i].address>>24)==0x81)assert(read_address_16bit(&cpu,codes[i].address)==(uint16_t)codes[i].value);
   /* Toggle repeatedly while paused: no VI and no loss of originals. */
   wide.enabled=0;wide.enabled=1;wide.enabled=0;cheat_apply_cheats(&ctx,&cpu,ENTRY_VI);
   for(unsigned i=0;i<n;i++)assert(read_address_16bit(&cpu,codes[i].address)==original[i]);
  }
  wide.enabled=1;cheat_apply_cheats(&ctx,&cpu,ENTRY_VI);
  update_address_16bit(&cpu,codes[0].address,0x1234);wide.enabled=0;cheat_apply_cheats(&ctx,&cpu,ENTRY_VI);
  assert(read_address_16bit(&cpu,codes[0].address)==0x1234);
  wide.enabled=1;cheat_apply_cheats(&ctx,&cpu,ENTRY_VI);assert(read_address_16bit(&cpu,codes[0].address)==0x1234);
  /* An unpatched state can be patched again; a patched state retains its originals. */
  update_address_16bit(&cpu,codes[0].address,original[0]);cheat_apply_cheats(&ctx,&cpu,ENTRY_VI);
  wide.enabled=0;cheat_apply_cheats(&ctx,&cpu,ENTRY_VI);assert(read_address_16bit(&cpu,codes[0].address)==original[0]);
  list_del(&wide.list);
 }
 assert(invalidates>0);free(ram.dram);puts("PASS live widescreen core: both profiles, 20 enable/restore cycles, paused toggles, boot zeros, overlay replacement and state values");
}
'''
Path('build/widescreen-36/test-core.c').write_text(prefix+definitions+helpers+apply+suffix)
