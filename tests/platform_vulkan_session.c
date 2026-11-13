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
