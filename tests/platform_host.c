#include "platform.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
static uint32_t framebuffer[SCREEN_W*SCREEN_H];
static unsigned polls;
static double now;
static bool saw_video;
static bool saw_second_video;
static unsigned hud_calls;
static bool telemetry_run;
static unsigned telemetry_checks;
static bool late_hud[2];
bool platform_init(void) {telemetry_run=getenv("TEST_PERF_HUD")!=NULL;return true;}
void platform_shutdown(void) {
    if(telemetry_run) {assert(telemetry_checks>=30);puts("PASS: sustained FPS HUD, target and speed independent of library time");return;}
    assert(saw_video && "N64 core must render test ROM, not only frontend menu");
    assert(saw_second_video && "Second ROM launch must render after deinit/init");
    assert(hud_calls>=20 && "FPS HUD must remain visible after ROM starts");
    assert(late_hud[0] && late_hud[1]);
    puts("PASS: frontend arranque ROM / video / pausa / estado / retorno / salida");
}
uint32_t *platform_framebuffer(void) { return framebuffer; }
void platform_poll(Input p[4]) {
    memset(p,0,4*sizeof(*p));p[0].connected=true; polls++;
    if(telemetry_run) {
        if(polls==200) p[0].buttons=BTN_CONFIRM;
        if(polls==360) p[0].buttons=BTN_PAUSE;
        if(polls==362 || polls==364 || polls==366 || polls==368) p[0].buttons=BTN_DOWN;
        if(polls==370) p[0].buttons=BTN_CONFIRM;
        if(polls==372) p[0].buttons=BTN_EXIT;
        assert(polls<380);return;
    }
    if(polls==1) p[0].buttons=BTN_HELP;
    if(polls==3) p[0].buttons=BTN_CONFIRM; // must not launch in controller test
    if(polls==5) p[0].buttons=BTN_BACK; // leave test, stay in library
    if(polls==7) p[0].buttons=BTN_SCAN;
    unsigned step=polls>8?polls-8:0;
    if(step==1) p[0].buttons=BTN_CONFIRM;
    if(step==15) p[0].buttons=BTN_PAUSE;
    if(step==16) p[0].buttons=BTN_UP; // final pause item: image profile
    if(step==17) p[0].buttons=BTN_CONFIRM; // switch VI while paused
    if(step==18) p[0].buttons=BTN_BACK; // resume and apply renderer change
    if(step==19) p[0].buttons=BTN_PAUSE;
    if(step==20) p[0].buttons=BTN_DOWN;
    if(step==21) p[0].buttons=BTN_CONFIRM; // save slot 0
    if(step==22) p[0].buttons=BTN_DOWN;
    if(step==23) p[0].buttons=BTN_CONFIRM; // load slot 0
    if(step==25 || step==27) p[0].buttons=BTN_DOWN;
    if(step==29) p[0].buttons=BTN_CONFIRM; // library
    if(step==31) p[0].buttons=BTN_CONFIRM; // launch same ROM again
    if(step==42) p[0].buttons=BTN_PAUSE;
    if(step==44 || step==46 || step==48 || step==50) p[0].buttons=BTN_DOWN;
    if(step==52) p[0].buttons=BTN_CONFIRM;
    if(step==54) p[0].buttons=BTN_EXIT;
    assert(polls<68);
}
bool platform_present(void) {
    if(polls>=11 && polls<23) {
        uint32_t c=framebuffer[SCREEN_H/2*SCREEN_W+SCREEN_W/2];
        if((c&0x00ffffff)==0x48dac1) saw_video=true;
    }
    if(polls>=41 && polls<50 && (framebuffer[SCREEN_H/2*SCREEN_W+SCREEN_W/2]&0x00ffffff)==0x48dac1) saw_second_video=true;
    now+=1.0/60;return true;
}
void platform_audio(const int16_t *s,size_t n,double rate) { (void)s;(void)n;(void)rate; }
void platform_audio_clear(void) {}
double platform_time(void) { return now; }
void platform_sleep(double seconds) { (void)seconds; }
void platform_text(int x,int y,const char *s,uint32_t c,int scale) {
    (void)x;(void)y;(void)c;(void)scale;
    if(!strncmp(s,"FPS ",4)) {
        hud_calls++;
        if(polls==20)late_hud[0]=true;
        if(polls==49)late_hud[1]=true;
        if(telemetry_run && polls>=300 && polls<350) {
            double fps,target,speed;
            assert(sscanf(s,"FPS %lf / %lf | velocidad %lf%%",&fps,&target,&speed)==3);
            assert(target>49 && target<61 && fps>target*0.95 && fps<target*1.05);
            assert(speed>95 && speed<105);telemetry_checks++;
        }
    }
}
