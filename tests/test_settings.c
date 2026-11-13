#define _POSIX_C_SOURCE 200809L
#include "settings.h"
#include "storage.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
static void write_cfg(const char *p,const char *s){assert(file_write_atomic(p,s,strlen(s)));}
int main(void){
 char root[]="/tmp/nps-settings-XXXXXX";assert(mkdtemp(root));char path[1024],dir[1024],gamepath[1024];
 snprintf(path,sizeof path,"%s/global.cfg",root);snprintf(dir,sizeof dir,"%s/games",root);
 const char *a="rom-0000000000000001",*b="rom-0000000000000002";
 Nps64AppSettings app,loaded;assert(!nps64_settings_load(path,&app));assert(app.hud_mode==2&&!app.defaults.widescreen&&app.defaults.filtered_vi);
 const char *legacy[]={"NPS64CFG1 2 1 7\n","NPS64CFG2 2 1 7 0\n","NPS64CFG3 2 1 7 0 2\n"};
 for(unsigned i=0;i<3;i++){write_cfg(path,legacy[i]);assert(nps64_settings_load(path,&app));assert(app.theme==2&&app.defaults.slot==7&&!app.defaults.widescreen);}
 app.hud_mode=0;assert(nps64_settings_save(path,&app));assert(nps64_settings_load(path,&loaded));assert(loaded.hud_mode==0&&loaded.defaults.controller_pak==2);
 Nps64GameSettings ga,gb;assert(!nps64_game_settings_load(dir,a,&app.defaults,&ga));ga.widescreen=true;ga.filtered_vi=true;ga.slot=3;ga.controller_pak=1;
 assert(nps64_game_settings_save(dir,a,&ga));assert(!nps64_game_settings_load(dir,b,&app.defaults,&gb));assert(!gb.widescreen&&gb.slot==7);
 assert(nps64_game_settings_save(dir,b,&gb));assert(nps64_game_settings_load(dir,a,&app.defaults,&ga));assert(ga.widescreen&&ga.filtered_vi&&ga.slot==3&&ga.controller_pak==1);
 assert(!nps64_game_settings_save(dir,"../global",&ga));assert(!nps64_game_settings_reset(dir,"../global"));
 assert(nps64_game_settings_reset(dir,a));assert(!nps64_game_settings_load(dir,a,&app.defaults,&ga));assert(!ga.widescreen);assert(nps64_game_settings_load(dir,b,&app.defaults,&gb));
 const char *bad[]={"NPS64CFG4 0 3 0 1 0","NPS64CFG4 0 1 8 1 0","NPS64CFG4 0 1 0 2 0","NPS64CFG4 0 1 0 1 3","NPS64CFG4 0 1","NPS64CFG4 0 1 0 1 0 garbage","NPS64CFG4 -1 1 0 1 0"};
 for(unsigned i=0;i<sizeof bad/sizeof *bad;i++){write_cfg(path,bad[i]);assert(!nps64_settings_load(path,&loaded));assert(loaded.hud_mode==2);}
 char huge[300];memset(huge,' ',sizeof huge);memcpy(huge,"NPS64CFG4 0 1 0 1 0",19);assert(file_write_atomic(path,huge,sizeof huge));assert(!nps64_settings_load(path,&loaded));
 const char nul[]="NPS64CFG4 0 1 0 1 0\0ignored";assert(file_write_atomic(path,nul,sizeof nul));assert(!nps64_settings_load(path,&loaded));
 snprintf(gamepath,sizeof gamepath,"%s/%s.cfg",dir,b);write_cfg(gamepath,"NPS64GAME1 2 1 0 0");assert(!nps64_game_settings_load(dir,b,&app.defaults,&gb));assert(!gb.widescreen&&gb.slot==7);
 unlink(gamepath);rmdir(dir);unlink(path);rmdir(root);puts("PASS settings: migration, HUD, per-ROM isolation, reset and malformed input");return 0;
}
