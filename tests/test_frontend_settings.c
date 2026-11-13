#define main frontend_main
#include "../src/frontend.c"
#undef main
#include <assert.h>
#include <unistd.h>
static uint32_t fb[SCREEN_W*SCREEN_H];static double now;static unsigned tick;static bool covers_paused;static unsigned hud_lines,detail_lines;
void covers_init(void){} void covers_shutdown(void){} void covers_refresh(void){}
void covers_set_paused(bool p){covers_paused=p;}
void covers_request(const Game *g){(void)g;}
void covers_draw(const Game *g,int x,int y,int w,int h){(void)g;(void)x;(void)y;(void)w;(void)h;}
void covers_status(char *s,size_t n){snprintf(s,n,"Caratulas locales");}
bool platform_init(void){return true;}void platform_shutdown(void){}
uint32_t *platform_framebuffer(void){return fb;}
void platform_text(int x,int y,const char*s,uint32_t c,int z){(void)x;(void)y;(void)c;(void)z;if(!strncmp(s,"FPS ",4))hud_lines++;if(!strncmp(s,"core ",5))detail_lines++;}
void platform_audio(const int16_t*s,size_t n,double rate){(void)s;(void)n;(void)rate;}
void platform_audio_clear(void){}void platform_rumble_clear(void){}
bool platform_rumble(unsigned p,unsigned e,uint16_t s){(void)p;(void)e;(void)s;return false;}
double platform_time(void){return now;}void platform_sleep(double s){(void)s;}
static void screenshot(const char *name){FILE*f=fopen(name,"wb");assert(f);fprintf(f,"P6\n1280 720\n255\n");for(unsigned i=0;i<SCREEN_W*SCREEN_H;i++){unsigned char rgb[]={fb[i]>>16,fb[i]>>8,fb[i]};fwrite(rgb,1,3,f);}fclose(f);}
bool platform_present(void){now+=1./60;return true;}
void platform_poll(Input p[4]){
 memset(p,0,4*sizeof(*p));p[0].connected=true;tick++;
 switch(tick){
 case 1:assert(game_count==2);p[0].buttons=BTN_UPDATE;break;
 case 3:assert(settings_page==1);screenshot("build/settings-general.ppm");p[0].buttons=BTN_CONFIRM;break;
 case 5:assert(preferences.hud_mode==0);p[0].buttons=BTN_BACK;break;
 case 7:screenshot("build/settings-library.ppm");p[0].buttons=BTN_GAME_SETTINGS;break;
 case 9:assert(settings_page==2);assert(!editing_game.widescreen);p[0].buttons=BTN_CONFIRM;break;
 case 11:assert(editing_game.widescreen);screenshot("build/settings-game.ppm");p[0].buttons=BTN_BACK;break;
 case 13:p[0].buttons=BTN_DOWN;break;
 case 15:p[0].buttons=BTN_GAME_SETTINGS;break;
 case 17:assert(settings_page==2&&!editing_game.widescreen);p[0].buttons=BTN_BACK;break;
 case 19:p[0].buttons=BTN_UP;break;
 case 21:p[0].buttons=BTN_CONFIRM;break;
 case 22:assert(running&&widescreen&&covers_paused);p[0].buttons=BTN_PAUSE;break;
 case 24:assert(paused);p[0].buttons=BTN_DOWN;break;
 case 26: p[0].buttons=BTN_DOWN;break;
 case 28: p[0].buttons=BTN_DOWN;break;
 case 30:screenshot("build/settings-pause.ppm");assert(pause_selected==3);p[0].buttons=BTN_CONFIRM;break;
 case 32:assert(!widescreen);p[0].buttons=BTN_DOWN;break;
 case 34:p[0].buttons=BTN_CONFIRM;break;
 case 36:assert(!running&&!covers_paused);p[0].buttons=BTN_GAME_SETTINGS;break;
 case 38:assert(!editing_game.widescreen);p[0].buttons=BTN_BACK;break;
 case 40:p[0].buttons=BTN_LEFT;break;
 case 42:assert(sidebar_focus);p[0].buttons=BTN_DOWN;break;
 case 44:p[0].buttons=BTN_CONFIRM;break;
 case 46:assert(settings_page==1&&!sidebar_focus);p[0].buttons=BTN_EXIT;break;
 default:assert(tick<48);
 }
}
int main(void){
 system("rm -rf build/ui-settings-test; mkdir -p build/ui-settings-test/games");
 unsigned char rom[4096]={0x80,0x37,0x12,0x40};FILE*f=fopen("build/testdata/games/NPS64-test.z64","rb");
 assert(f);assert(fread(rom,1,sizeof rom,f)==sizeof rom);fclose(f);
 assert(file_write_atomic(DATA_ROOT "/games/A.z64",rom,sizeof rom));rom[100]^=1;assert(file_write_atomic(DATA_ROOT "/games/B.z64",rom,sizeof rom));
 assert(frontend_main()==0);Nps64AppSettings stored;assert(nps64_settings_load(CONFIG_PATH,&stored)&&stored.hud_mode==0);
 native_width=2;native_height=2;native_format=RETRO_PIXEL_FORMAT_XRGB8888;native_frame=malloc(16);assert(native_frame);for(unsigned i=0;i<4;i++)((uint32_t*)native_frame)[i]=0xff336699;
 widescreen=false;game_video_draw();assert(fb[360*SCREEN_W+100]==0xff000000&&fb[360*SCREEN_W+640]==0xff336699);
 widescreen=true;game_video_draw();assert(fb[360*SCREEN_W+100]==0xff336699);
 preferences.hud_mode=0;game_hud_draw();assert(hud_lines==0&&detail_lines==0);
 preferences.hud_mode=1;game_hud_draw();assert(hud_lines==1&&detail_lines==0);
 preferences.hud_mode=2;game_hud_draw();assert(hud_lines==2&&detail_lines==1);free(native_frame);native_frame=NULL;
 puts("PASS frontend: sidebar, general HUD, per-game aspect isolation, launch, pause change and persistence");return 0;
}
