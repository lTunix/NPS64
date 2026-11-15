#define main frontend_main
#include "../src/frontend.c"
#undef main
#include <assert.h>
#include <unistd.h>
#ifdef NPS64_UPDATES
static char updater_test_text[160]="You have the latest version: 00.71";
void nps64_updater_init(void){}
void nps64_updater_request(void){snprintf(updater_test_text,sizeof updater_test_text,"Checking GitHub for updates...");}
void nps64_updater_shutdown(void){}
void nps64_updater_status(char *text,size_t capacity,bool *busy){if(capacity)snprintf(text,capacity,"%s",updater_test_text);if(busy)*busy=false;}
#endif
static unsigned cheat_enables,cheat_disables;
void __wrap_retro_cheat_reset(void){}
void __wrap_retro_cheat_set(unsigned index,bool enabled,const char *code){(void)index;if(enabled){assert(nps64_cheat_code_valid(code));cheat_enables++;}else{assert(!code);cheat_disables++;}}
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
bool platform_present(void){
#ifdef NPS64_UPDATES
 if(!strcmp(message,nps64_tr("Leyendo ROM. Espera..."))||!strcmp(message,nps64_tr("Partida guardada. Selecciona otro juego"))){char status[160];assert(!strcmp(current_message(status,sizeof status),message));}
#endif
 now+=1./60;return true;}
void platform_poll(Input p[4]){
 memset(p,0,4*sizeof(*p));p[0].connected=true;tick++;
 switch(tick){
 case 1:assert(game_count==2);p[0].buttons=BTN_UPDATE;break;
 case 3:assert(settings_page==1);screenshot("build/settings-general.ppm");p[0].buttons=BTN_CONFIRM;break;
 case 5:assert(preferences.hud_mode==0);p[0].buttons=BTN_BACK;break;
 case 7:screenshot("build/settings-library.ppm");p[0].buttons=BTN_GAME_SETTINGS;break;
 case 9:assert(settings_page==2);assert(!editing_game.widescreen);p[0].buttons=BTN_CONFIRM;break;
 case 11:assert(editing_game.widescreen);editing_game.resolution=0;assert(nps64_game_settings_save(GAME_SETTINGS_DIR,games[selected].key,&editing_game));screenshot("build/settings-game.ppm");p[0].buttons=BTN_BACK;break;
 case 13:p[0].buttons=BTN_DOWN;break;
 case 15:p[0].buttons=BTN_GAME_SETTINGS;break;
 case 17:assert(settings_page==2&&!editing_game.widescreen&&editing_game.resolution==1);p[0].buttons=BTN_BACK;break;
 case 19:p[0].buttons=BTN_UP;break;
 case 21:p[0].buttons=BTN_CONFIRM;break;
 case 22:assert(resolution_mode==0&&resolution_next==0);assert(running&&widescreen&&covers_paused&&ram_writes_allowed);p[0].buttons=BTN_PAUSE;break;
 case 24:assert(paused);p[0].buttons=BTN_DOWN;break;
 case 26: p[0].buttons=BTN_DOWN;break;
 case 28: p[0].buttons=BTN_DOWN;break;
 case 30:screenshot("build/settings-pause.ppm");assert(pause_selected==3);p[0].buttons=BTN_CONFIRM;break;
 case 31:pause_selected=PAUSE_CHOICES-3;p[0].buttons=BTN_RIGHT;break;
 case 32:assert(resolution_mode==0&&resolution_next==1);pause_selected=3;assert(!widescreen);p[0].buttons=BTN_DOWN;break;
 case 34:p[0].buttons=BTN_CONFIRM;break;
 case 36:assert(!running&&!covers_paused);assert(!strcmp(message,nps64_tr("Partida guardada. Selecciona otro juego")));p[0].buttons=BTN_GAME_SETTINGS;break;
 case 38:assert(!editing_game.widescreen&&editing_game.resolution==1);p[0].buttons=BTN_BACK;break;
 case 40:p[0].buttons=BTN_LEFT;break;
 case 42:assert(sidebar_focus);p[0].buttons=BTN_DOWN;break;
 case 44:p[0].buttons=BTN_CONFIRM;break;
 case 46:assert(settings_page==1&&!sidebar_focus);assert(preferences.language==NPS64_LANG_ENGLISH);settings_row=8;p[0].buttons=BTN_CONFIRM;break;
 case 48:assert(preferences.language==NPS64_LANG_SPANISH);assert(!strcmp(nps64_tr("Configuracion"),"Configuracion"));screenshot("build/settings-spanish.ppm");{Nps64AppSettings saved;assert(nps64_settings_load(CONFIG_PATH,&saved)&&saved.language==NPS64_LANG_SPANISH);}p[0].buttons=BTN_CONFIRM;break;
 case 50:assert(preferences.language==NPS64_LANG_ENGLISH);assert(!strcmp(nps64_tr("Configuracion"),"Settings"));screenshot("build/settings-english.ppm");settings_page=2;settings_row=7;p[0].buttons=BTN_CONFIRM;break;
 case 52:assert(cheat_screen&&game_cheats.count==3);screenshot("build/cheats-library.ppm");p[0].buttons=BTN_CONFIRM;break;
 case 54:assert(game_cheats.items[0].enabled);p[0].buttons=BTN_DOWN;break;
 case 56:p[0].buttons=BTN_CONFIRM;break;
 case 58:assert(!game_cheats.items[0].enabled&&game_cheats.items[1].enabled);p[0].buttons=BTN_BACK;break;
 case 60:assert(!cheat_screen);p[0].buttons=BTN_BACK;break;
 case 62:p[0].buttons=BTN_CONFIRM;break;
 case 64:assert(running&&cheat_enables==1);p[0].buttons=BTN_PAUSE;break;
 case 66:screenshot("build/pause-cheats-entry.ppm");pause_selected=PAUSE_CHOICES-4;p[0].buttons=BTN_CONFIRM;break;
 case 68:assert(cheat_screen&&paused&&game_cheats.items[1].enabled);screenshot("build/cheats-pause.ppm");p[0].buttons=BTN_SCAN;break;
 case 70:assert(!nps64_cheats_enabled(&game_cheats)&&cheat_disables==1);p[0].buttons=BTN_BACK;break;
 case 72:p[0].buttons=BTN_BACK;break;
 case 74:assert(!paused);p[0].buttons=BTN_PAUSE;break;
 case 76:pause_selected=4;p[0].buttons=BTN_CONFIRM;break;
 case 78:assert(!running);p[0].buttons=BTN_EXIT;break;
 default:assert(tick<80);
 }
}
int main(void){
 system("rm -rf build/ui-settings-test; mkdir -p build/ui-settings-test/games");
 unsigned char rom[4096]={0x80,0x37,0x12,0x40};FILE*f=fopen("build/testdata/games/NPS64-test.z64","rb");
 assert(f);assert(fread(rom,1,sizeof rom,f)==sizeof rom);fclose(f);
 assert(file_write_atomic(DATA_ROOT "/games/A.z64",rom,sizeof rom));rom[100]^=1;assert(file_write_atomic(DATA_ROOT "/games/B.z64",rom,sizeof rom));
 char cheat_identity[21];snprintf(cheat_identity,sizeof cheat_identity,"%02X%02X%02X%02X-%02X%02X%02X%02X-%02X",rom[16],rom[17],rom[18],rom[19],rom[20],rom[21],rom[22],rom[23],rom[62]);
 FILE*db=fopen(NPS64_CHEAT_DATABASE,"wb");assert(db);fprintf(db,"NPS64CHEATS1\nR|%s|Fixture\nC|0000000000000001|0000000000000001|Infinite health: 10|80000010 000A\nC|0000000000000002|0000000000000001|Infinite health: 20|80000010 0014\nC|0000000000000003|0000000000000003|Infinite lives|80000020 0009\n",cheat_identity);fclose(db);
 assert(frontend_main()==0);
#ifdef NPS64_UPDATES
 char status[160];
 notify("Saved game status");assert(!strcmp(current_message(status,sizeof status),"Saved game status"));
 strcpy(updater_test_text,"A background download completed");assert(!strcmp(current_message(status,sizeof status),"Saved game status"));
 request_updates();assert(!strcmp(current_message(status,sizeof status),"Checking GitHub for updates..."));
 notify("Reading ROM status");assert(!strcmp(current_message(status,sizeof status),"Reading ROM status"));
 request_updates();updater_test_text[0]=0;assert(!strcmp(current_message(status,sizeof status),"Reading ROM status"));
 puts("PASS status priority: current version cannot mask load/save, late background results stay secondary, manual check reselects updater, empty status falls back");
#endif

#ifdef NPS64_GPU_PRESENT_TEST
 gpu_prepared=true;gpu_fresh=true;native_width=320;native_height=240;native_borrowed=(const unsigned char*)fb;
 uint64_t old_frames=video_frames;video_refresh(NULL,0,0,0);
 assert(gpu_prepared&&gpu_fresh&&native_width==320&&native_height==240&&native_borrowed==(const unsigned char*)fb&&video_frames==old_frames);
 gpu_prepared=false;gpu_fresh=false;native_borrowed=NULL;
#endif
 use_gpu=true;resolution_mode=0;assert(!strcmp(override("parallel-n64-upscaling"),"1x"));resolution_mode=1;assert(!strcmp(override("parallel-n64-upscaling"),"2x"));char previous_key[sizeof current.key];memcpy(previous_key,current.key,sizeof previous_key);
 strcpy(current.key,"rom-cf6bb875013bfa0c");assert(!strcmp(override("parallel-n64-parallel-rdp-synchronous"),"enabled"));
 resolution_mode=0;assert(!strcmp(override("parallel-n64-parallel-rdp-synchronous"),"enabled"));
 strcpy(current.key,"rom-0000000000000000");resolution_mode=1;assert(!strcmp(override("parallel-n64-parallel-rdp-synchronous"),"enabled"));
 memcpy(current.key,previous_key,sizeof previous_key);use_gpu=false;Nps64AppSettings stored;assert(nps64_settings_load(CONFIG_PATH,&stored)&&stored.hud_mode==0);
 native_width=2;native_height=2;native_format=RETRO_PIXEL_FORMAT_XRGB8888;native_frame=malloc(16);assert(native_frame);for(unsigned i=0;i<4;i++)((uint32_t*)native_frame)[i]=0xff336699;
 widescreen=false;game_video_draw();assert(fb[360*SCREEN_W+100]==0xff000000&&fb[360*SCREEN_W+640]==0xff336699);
 widescreen=true;game_video_draw();assert(fb[360*SCREEN_W+100]==0xff336699);
 preferences.hud_mode=0;game_hud_draw();assert(hud_lines==0&&detail_lines==0);
 preferences.hud_mode=1;game_hud_draw();assert(hud_lines==1&&detail_lines==0);
 preferences.hud_mode=2;game_hud_draw();assert(hud_lines==2&&detail_lines==1);free(native_frame);native_frame=NULL;
 puts("PASS frontend: sidebar, English default, live English/Spanish switch and persistence, cheat library/pause toggles and persistence, variant exclusion, general HUD, per-game isolation, launch and pause");return 0;
}
