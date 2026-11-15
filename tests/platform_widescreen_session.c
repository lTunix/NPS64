/* Actual ROM integration: drive the pause menu, then inspect projection writes. */
#define platform_poll benchmark_poll
#include "platform_bench.c"
#undef platform_poll
#include "widescreen.h"
#include "log.h"
#include <libretro.h>
#include <assert.h>
static unsigned wide_frames,wide_stage,wide_action;
static uint32_t wide_addresses[16];
static uint16_t wide_originals[16],wide_values[16];
static unsigned wide_count;
void platform_poll(Input pads[4]){
 benchmark_poll(pads);
 if(wide_stage<4&&wide_frames>=300+wide_stage*120){
  static const unsigned enable[]={BTN_PAUSE,0,BTN_DOWN,0,BTN_DOWN,0,BTN_DOWN,0,BTN_CONFIRM,0,BTN_CONFIRM,0,BTN_BACK,0};
  static const unsigned disable[]={BTN_PAUSE,0,BTN_DOWN,0,BTN_DOWN,0,BTN_DOWN,0,BTN_CONFIRM,0,BTN_BACK,0};
  const unsigned *actions=(wide_stage&1)?disable:enable;
  unsigned count=(wide_stage&1)?sizeof(disable)/sizeof(*disable):sizeof(enable)/sizeof(*enable);
  pads[0].buttons=actions[wide_action++];
  if(wide_action==count){wide_stage++;wide_action=0;}
 }else if(wide_stage==4&&wide_frames>=780){
  static const unsigned leave[]={BTN_PAUSE,0,BTN_DOWN,0,BTN_DOWN,0,BTN_DOWN,0,BTN_DOWN,0,BTN_CONFIRM,0,BTN_EXIT};
  assert(wide_action<sizeof(leave)/sizeof(*leave));pads[0].buttons=leave[wide_action++];
 }
}
void __real_retro_run(void);
void __wrap_retro_run(void){
 unsigned char *ram=retro_get_memory_data(RETRO_MEMORY_SYSTEM_RAM);assert(ram);
 if(wide_frames==299){
  const Nps64WideProfile *p=&nps64_wide_profiles[1];const char *text=p->code;
  while(*text){unsigned a,v;int used=0;assert(sscanf(text,"%x %x %n",&a,&v,&used)==2&&used>0&&wide_count<16);text+=used;
   assert((a>>24)==0x81);wide_addresses[wide_count]=a&0xffffff;wide_values[wide_count]=(uint16_t)v;
   wide_originals[wide_count]=*(uint16_t*)(ram+(wide_addresses[wide_count]^2));
   nps64_log("[wide-test] before address=%08x original=%04x patch=%04x\n",a,wide_originals[wide_count],v);wide_count++;
  }
 }
 __real_retro_run();
 if(wide_frames==310||wide_frames==430||wide_frames==550||wide_frames==670){
  int active=wide_frames==310||wide_frames==550;
  for(unsigned i=0;i<wide_count;i++){
   uint16_t actual=*(uint16_t*)(ram+(wide_addresses[i]^2)),expected=active?wide_values[i]:wide_originals[i];
   nps64_log("[wide-test] frame=%u active=%d address=%08x actual=%04x expected=%04x\n",wide_frames,active,wide_addresses[i],actual,expected);
   assert(actual==expected);
  }
  nps64_log("[wide-test] PASS live %s all %u halfwords\n",active?"enable":"restore",wide_count);
 }
 wide_frames++;
}
