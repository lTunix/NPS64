/* Native timed-session test: attempt resume after the cap, then leave cleanly. */
#define platform_poll benchmark_poll
#include "platform_bench.c"
#undef platform_poll
#include "log.h"
#ifndef NPS64_GAME_TEST_SECONDS
#error Set the native test duration explicitly
#endif
#ifndef NPS64_SESSION_TEST_SECONDS
#define NPS64_SESSION_TEST_SECONDS NPS64_GAME_TEST_SECONDS
#endif
void platform_poll(Input pads[4]) {
    static double launched;
    static unsigned exit_step;
    benchmark_poll(pads);
    /* Poll 200 requests loading; loading/first shader compilation can take
     * seconds. Start after it returns so the resume attempt follows the cap. */
    if(polls==201)launched=platform_time();
    if(launched && platform_time()-launched>=NPS64_SESSION_TEST_SECONDS+2.0) {
        static const unsigned actions[]={0,BTN_PAUSE,0,BTN_DOWN,0,BTN_DOWN,0,BTN_DOWN,0,BTN_DOWN,0,BTN_CONFIRM,0,BTN_EXIT};
        pads[0].buttons=exit_step<sizeof(actions)/sizeof(*actions)?actions[exit_step]:BTN_EXIT;
        if(exit_step==1)nps64_log("[session-test] attempt resume after deadline\n");
        exit_step++;
    }
}
#ifdef NPS64_CHEAT_CORE_TEST
#include <libretro.h>
#include <assert.h>
void __real_retro_run(void);
void __wrap_retro_run(void){
    static unsigned frame;static unsigned char old;static size_t offset;static unsigned char *ram;
    if(frame==70){
        size_t size=retro_get_memory_size(RETRO_MEMORY_SYSTEM_RAM);ram=retro_get_memory_data(RETRO_MEMORY_SYSTEM_RAM);assert(ram&&size>=4*1024*1024);
        offset=size-16;old=ram[offset^3];char code[32];snprintf(code,sizeof code,"%08X 00B7",0x80000000u|(unsigned)offset);retro_cheat_set(4095,true,code);
    }
    if(frame==71)retro_cheat_set(4095,false,NULL);
    __real_retro_run();
    if(frame==70){assert(ram[offset^3]==0xb7);nps64_log("[cheat-core-test] live write applied\n");}
    if(frame==71){assert(ram[offset^3]==old);nps64_log("[cheat-core-test] live disable restored original byte\n");}
    frame++;
}
#endif
