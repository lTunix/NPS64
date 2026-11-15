/* Drive the actual pause-menu resolution row without reloading the ROM. */
#define platform_poll benchmark_poll
#include "platform_bench.c"
#undef platform_poll
#include "log.h"
#include <libretro.h>
#include <assert.h>
static unsigned resolution_frames,resolution_stage,resolution_action,resolution_calls;
static unsigned last_factor=2;
void platform_poll(Input pads[4]){
 benchmark_poll(pads);
 if(resolution_stage<6&&resolution_frames>=240+resolution_stage*90){
  /* Ten-row pause menu without updater: resolution is row 7; wrap up. */
  static const unsigned actions[]={BTN_PAUSE,0,BTN_UP,0,BTN_UP,0,BTN_UP,0,BTN_CONFIRM,0,BTN_BACK,0};
  pads[0].buttons=actions[resolution_action++];
  if(resolution_action==sizeof(actions)/sizeof(*actions)){resolution_stage++;resolution_action=0;}
 }else if(resolution_stage==6&&resolution_frames>=840){
  static const unsigned leave[]={BTN_PAUSE,0,BTN_DOWN,0,BTN_DOWN,0,BTN_DOWN,0,BTN_DOWN,0,BTN_CONFIRM,0,BTN_EXIT};
  assert(resolution_action<sizeof(leave)/sizeof(*leave));pads[0].buttons=leave[resolution_action++];
 }
}
int __real_nps64_parallel_resolution(unsigned factor);
static uint64_t memory_hash(const void *data,size_t n){const unsigned char*p=data;uint64_t h=UINT64_C(1469598103934665603);for(size_t i=0;i<n;i++){h^=p[i];h*=UINT64_C(1099511628211);}return h;}
int __wrap_nps64_parallel_resolution(unsigned factor){
 assert(factor==(last_factor==2?1u:2u));
 const void*ram=retro_get_memory_data(RETRO_MEMORY_SYSTEM_RAM);size_t size=retro_get_memory_size(RETRO_MEMORY_SYSTEM_RAM);assert(ram&&size>=4*1024*1024);
 const void*sram=retro_get_memory_data(RETRO_MEMORY_SAVE_RAM);size_t sram_size=retro_get_memory_size(RETRO_MEMORY_SAVE_RAM);
 uint64_t before=memory_hash(ram,size),save_before=sram?memory_hash(sram,sram_size):0;
 unsigned before_frames=resolution_frames;
 assert(__real_nps64_parallel_resolution(factor)==1);
 assert(retro_get_memory_data(RETRO_MEMORY_SYSTEM_RAM)==ram&&retro_get_memory_size(RETRO_MEMORY_SYSTEM_RAM)==size);
 assert(memory_hash(ram,size)==before&&resolution_frames==before_frames);
 assert(retro_get_memory_data(RETRO_MEMORY_SAVE_RAM)==sram&&(!sram||memory_hash(sram,sram_size)==save_before));
 last_factor=factor;resolution_calls++;
 nps64_log("[resolution-test] PASS factor=%u RAM_bytes=%zu SRAM_bytes=%zu preserved=1 call=%u\n",factor,size,sram_size,resolution_calls);return 1;
}
void __real_retro_run(void);
void __wrap_retro_run(void){__real_retro_run();resolution_frames++;if(resolution_frames==840){assert(resolution_calls==6);nps64_log("[resolution-test] PASS six live switches and continuous core execution\n");}}
