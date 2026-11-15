"""Compile the actual patched memory-access helpers against bounded test RAM."""
from pathlib import Path
source=Path('build/gpu-game-test/core/mupen64plus-core/src/main/cheat.c').read_text()
body=source[source.index('static uint16_t read_address_16bit'):source.index('static cheat_t *find_or_create_cheat')]
prefix=r'''
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#define S8 3
#define S16 2
#define CHEAT_CODE_MAGIC_VALUE UINT32_C(0xdead0000)
struct ram {void*dram;size_t dram_size;};
struct r4300_core {struct ram*rdram;};
static unsigned invalidates;
static void invalidate_r4300_cached_code(struct r4300_core*c,uint32_t address,unsigned length){(void)c;(void)address;(void)length;invalidates++;}
'''
suffix=r'''
int main(void){
 struct ram ram={calloc(1,4*1024*1024),4*1024*1024};assert(ram.dram);struct r4300_core cpu={&ram};uint32_t old=CHEAT_CODE_MAGIC_VALUE;
 assert(execute_cheat(&cpu,0x80000010,0x42,&old));assert(read_address_8bit(&cpu,0x80000010)==0x42&&old==0&&invalidates==1);
 assert(execute_cheat(&cpu,0xd0000010,0x42,NULL));assert(!execute_cheat(&cpu,0xd0000010,0x41,NULL));
 assert(execute_cheat(&cpu,0x81000020,0xabcd,NULL));assert(read_address_16bit(&cpu,0x81000020)==0xabcd);
 unsigned before=invalidates;
 const uint32_t bad[]={0x80400000,0x80ffffff,0x81400000,0x813fffff,0x81000021,0xd1400000,0xd13fffff};
 for(unsigned i=0;i<sizeof bad/sizeof *bad;i++)assert(!execute_cheat(&cpu,bad[i],0xbeef,NULL));assert(invalidates==before);
 assert(execute_cheat(&cpu,0x813ffffe,0xffff,NULL));free(ram.dram);puts("PASS actual cheat RAM helpers: writes, conditions, JIT invalidation, 4 MiB boundary and unaligned/out-of-range rejection");return 0;
}
'''
Path('build/cheats-33/test-core-memory.c').write_text(prefix+body+suffix)
