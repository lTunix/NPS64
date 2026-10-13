/* Headless performance backend: real frontend, core and supplied local ROM. */
#include "platform.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "device/device.h"
extern struct device g_dev;
#ifdef NPS64_HLE_GAME
extern bool nps64_hle_sequence_active;
extern bool nps64_hle_is_live(void);
#endif

static uint32_t fb[SCREEN_W*SCREEN_H];
static unsigned char font[128*8*16];
static unsigned polls,presents,game_presents,lit_frames;
static double last_present,menu_ms,game_ms,worst_menu,worst_game;
static unsigned menu_samples,game_samples;
static double race_ms,worst_race;
static unsigned race_samples;
static unsigned limit=180;
static size_t live_audio_frames;
static bool hle_capture_pending,hle_capture_saved,hle_capture_last;
static bool race;
bool platform_init(void) {
    FILE *f=fopen("assets/font.bin","rb"); if(!f) return false;
    bool ok=fread(font,1,sizeof(font),f)==sizeof(font); fclose(f);
    const char *n=getenv("BENCH_FRAMES"); if(n) limit=(unsigned)atoi(n);
    race=getenv("BENCH_RACE")!=NULL;
    last_present=platform_time(); return ok;
}
void platform_shutdown(void) {
    printf("BENCH live_audio_frames=%zu\n",live_audio_frames);
    printf("BENCH menu_samples=%u avg_ms=%.3f max_ms=%.3f\n",menu_samples,menu_ms/(menu_samples?menu_samples:1),worst_menu);
    printf("BENCH game_samples=%u avg_ms=%.3f max_ms=%.3f effective_fps=%.2f lit_frames=%u\n",game_samples,game_ms/(game_samples?game_samples:1),worst_game,game_ms?1000.0*game_samples/game_ms:0,lit_frames);
    if(race_samples)printf("BENCH race_samples=%u avg_ms=%.3f max_ms=%.3f effective_fps=%.2f\n",race_samples,race_ms/race_samples,worst_race,1000.0*race_samples/race_ms);
}
uint32_t *platform_framebuffer(void) { return fb; }
void platform_poll(Input pads[4]) {
    memset(pads,0,4*sizeof(*pads)); pads[0].connected=true;
#ifdef NPS64_HLE_GAME
    if(nps64_hle_sequence_active)return;
#endif
    polls++;
    if(polls<200 && (polls&1)) pads[0].buttons=BTN_DOWN;
    if(polls==200) pads[0].buttons=BTN_CONFIRM;
    unsigned f=polls>=200?polls-200:0;
    if(getenv("BENCH_SM64")) {
        if(f>=400&&f<420)pads[0].joy=1u<<3;
        if((f>=500&&f<520)||(f>=600&&f<620))pads[0].joy=1u<<8;
    }
    if(getenv("BENCH_ZELDA_MENU")) {
        if(f>=1500&&f<1560)pads[0].joy=1u<<3; /* title -> file selection */
    }
    if(getenv("BENCH_GOLDENEYE")) {
        if(f>=1000&&f<1800&&(f-1000)%300<60)pads[0].joy=1u<<3;
        if(f>=2000&&f<3900&&(f-2000)%300<60)pads[0].joy=1u<<0; /* default N64 A */
        if((f>=4200&&f<4260)||(f>=4500&&f<4560))pads[0].joy=1u<<0;
        if(f>=4800)pads[0].ly=-12000; /* advance into Dam after the menu */
    }
    if(race) {
        if(f>=900 && f<920) pads[0].joy=1u<<3; /* title Start */
        /* Hold long enough for the game's 25 Hz input cadence. */
        if(f>=960 && f<2060 && (f-960)%120<20) pads[0].joy=1u<<0;
        if(f>=2200) pads[0].joy=1u<<0; /* accelerate */
    }
#ifdef NPS64_HLE_GAME
    const char *profile=getenv("BENCH_PROFILE_MODE");
    if(profile){
        if(f==600)pads[0].buttons=BTN_PAUSE;
        if(f==601)pads[0].buttons=!strcmp(profile,"vulkan")?BTN_HELP:BTN_BACK;
    }
#endif
    if(polls==200+limit) pads[0].buttons=BTN_PAUSE;
    if(polls>200+limit && polls<210+limit && !(polls&1)) pads[0].buttons=BTN_DOWN;
    if(polls==210+limit) pads[0].buttons=BTN_CONFIRM;
    if(polls==212+limit) pads[0].buttons=BTN_EXIT;
    if(getenv("BENCH_HLE_PREVIEW")&&polls>200+limit) {
        static const unsigned sequence[]={0,BTN_HELP,BTN_BACK,0,BTN_BACK,BTN_PAUSE,BTN_DOWN,0,BTN_DOWN,0,BTN_DOWN,0,BTN_DOWN,0,BTN_CONFIRM,BTN_EXIT};
        unsigned index=polls-(200+limit);
        pads[0].buttons=index<sizeof(sequence)/sizeof(sequence[0])?sequence[index]:BTN_EXIT;
        if(getenv("BENCH_EXPECT_NO_GEOMETRY")){
            if(index==2)pads[0].buttons=0;
            if(index==3)pads[0].buttons=BTN_HELP;
        }
    }
    if(getenv("BENCH_HLE_LIVE")&&polls>200+limit){
        unsigned i=polls-(200+limit);pads[0].buttons=0;
        if(i==1||i==603||i==635)pads[0].buttons=BTN_HELP;
        if(i==301||i==601||i==633||i==901||i==1001||i==1201)pads[0].buttons=BTN_PAUSE;
        if(i==303||i==905||i==1007)pads[0].buttons=BTN_BACK;
        if(i==903||i==1003||i==1005||i==1203||i==1205||i==1207||i==1209)pads[0].buttons=BTN_DOWN;
        if(i==904||i==1006||i==1211)pads[0].buttons=BTN_CONFIRM;
        if(i==1213)pads[0].buttons=BTN_CONFIRM;
        if(i==1215||i==1801)pads[0].buttons=BTN_PAUSE;
        if(i==1217)pads[0].buttons=BTN_HELP;
        if(i==1803||i==1805||i==1807||i==1809)pads[0].buttons=BTN_DOWN;
        if(i==1811)pads[0].buttons=BTN_CONFIRM;
        if(i>=1813)pads[0].buttons=BTN_EXIT;
        if(i>10&&i<250)pads[0].joy=1u<<8;
    }
}
static void screenshot(unsigned frame) {
    char path[100]; snprintf(path,sizeof(path),"build/bench-frame-%u.ppm",frame);
    FILE *f=fopen(path,"wb"); if(!f) return;
    fprintf(f,"P6\n%d %d\n255\n",SCREEN_W,SCREEN_H);
    for(size_t i=0;i<SCREEN_W*SCREEN_H;i++) {
        unsigned char rgb[3]={fb[i]>>16,fb[i]>>8,fb[i]}; fwrite(rgb,1,3,f);
    }
    fclose(f);
}
bool platform_present(void) {
    if(hle_capture_pending&&!hle_capture_saved){screenshot(999);hle_capture_saved=true;}
    if(hle_capture_last){screenshot(1000);hle_capture_last=false;}
    double now=platform_time(),ms=(now-last_present)*1000; last_present=now; presents++;
    if(getenv("BENCH_CPU_PROFILE")&&polls>200&&polls<200+limit){
        printf("PROFILE frame=%u ms=%.6f vulkan=0\n",polls-200,ms);
        last_present=platform_time();
    }
    if(polls>2 && polls<200) {menu_samples++;menu_ms+=ms;if(ms>worst_menu)worst_menu=ms;}
    if(polls>200 && polls<200+limit) {
        game_presents++; game_samples++;game_ms+=ms;if(ms>worst_game)worst_game=ms;
        if(race && game_presents>=2600) {race_samples++;race_ms+=ms;if(ms>worst_race)worst_race=ms;}
        unsigned lit=0; for(size_t i=0;i<SCREEN_W*SCREEN_H;i+=16) if(fb[i]&0x00ffffff) lit++;
        if(lit) lit_frames++;
        if(game_presents%30==0) {
            printf("BENCH progress frame=%u ms=%.3f lit_pixels=%u\n",game_presents,ms,lit); fflush(stdout);
            printf("STATE pc=%08x count=%08x status=%08x cause=%08x VI=%08x origin=%08x width=%u SP=%08x\n",*r4300_pc(&g_dev.r4300),g_dev.r4300.cp0.regs[9],g_dev.r4300.cp0.regs[12],g_dev.r4300.cp0.regs[13],g_dev.vi.regs[0],g_dev.vi.regs[1],g_dev.vi.regs[2],g_dev.sp.regs[4]); fflush(stdout);

            last_present=platform_time(); /* exclude capture and reporting I/O */
        }
        if(game_presents==300 || game_presents==limit-30 ||
           (getenv("BENCH_CAPTURE_EVERY") && game_presents%600==0)) {
            screenshot(game_presents);last_present=platform_time();
        }
    }
#ifdef NPS64_HLE_GAME
    if(getenv("BENCH_PROFILE_MODE")&&polls>200&&polls<200+limit){
        printf("PROFILE frame=%u ms=%.6f vulkan=%u\n",polls-200,ms,nps64_hle_is_live()?1:0);
        last_present=platform_time(); /* Exclude measurement output from the next sample. */
    }
#endif
    return true;
}
void platform_audio(const int16_t *s,size_t n,double r) {
    (void)s;(void)n;(void)r;
#ifdef NPS64_HLE_GAME
    if(nps64_hle_is_live())live_audio_frames+=n;
#endif
}
void platform_audio_clear(void) {}
double platform_time(void) {struct timespec t;clock_gettime(CLOCK_MONOTONIC,&t);return t.tv_sec+t.tv_nsec/1e9;}
void platform_sleep(double s) {(void)s; /* measure uncapped work, not simulated vsync */}
void platform_text(int x,int y,const char *text,uint32_t color,int scale) {
    if(getenv("BENCH_HLE_LIVE")&&strstr(text,"Vulkan + ref CPU"))hle_capture_pending=true;
    if(getenv("BENCH_HLE_PREVIEW")&&!strncmp(text,"NPS64 00.38 - Geometria",22))hle_capture_pending=true;
    if(getenv("BENCH_HLE_PREVIEW")&&strstr(text,"frame 64/64"))hle_capture_last=true;
    for(;*text;text++,x+=8*scale) {
        unsigned c=(unsigned char)*text;if(c>127)c='?';
        for(int gy=0;gy<16;gy++)for(int gx=0;gx<8;gx++) {
            if(font[c*128+gy*8+gx]<80)continue;
            for(int dy=0;dy<scale;dy++)for(int dx=0;dx<scale;dx++) {
                int px=x+gx*scale+dx,py=y+gy*scale+dy;
                if(px>=0&&py>=0&&px<SCREEN_W&&py<SCREEN_H)fb[py*SCREEN_W+px]=color;
            }
        }
    }
}

bool platform_rumble(unsigned port,unsigned effect,uint16_t strength) {(void)port;(void)effect;(void)strength;return false;}
void platform_rumble_clear(void) {}
