#include "settings.h"
#include "storage.h"
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>
static bool read_text(const char *path,char text[256]) {
    FILE *f=fopen(path,"rb");if(!f)return false;
    size_t n=fread(text,1,255,f);unsigned char extra;
    bool ok=n>0 && !ferror(f) && fread(&extra,1,1,f)==0 && !ferror(f);
    fclose(f);text[n]=0;return ok && !memchr(text,0,n);
}
static bool tail(const char *p){return strspn(p," \t\r\n")==strlen(p);}
static bool valid(const Nps64GameSettings *s){return s->slot<8 && s->controller_pak<3;}
void nps64_settings_defaults(Nps64AppSettings *s){*s=(Nps64AppSettings){.hud_mode=2,.defaults={.filtered_vi=true}};}
bool nps64_settings_load(const char *path,Nps64AppSettings *out) {
    char text[256],format[20];nps64_settings_defaults(out);if(!read_text(path,text))return false;
    unsigned t=0,w=0,slot=0,f=1,pak=0,hud=2;int end=0,n;
    if(sscanf(text,"%19s",format)!=1)return false;
    if(!strcmp(format,"NPS64CFG4")) {
        n=sscanf(text,"NPS64CFG4 %u %u %u %u %u %n",&t,&hud,&slot,&f,&pak,&end);
        if(n!=5 || !tail(text+end) || hud>2)return false;
    } else if(!strcmp(format,"NPS64CFG3")) {
        n=sscanf(text,"NPS64CFG3 %u %u %u %u %u %n",&t,&w,&slot,&f,&pak,&end);
        if(n!=5 || !tail(text+end) || w>1)return false;
    } else if(!strcmp(format,"NPS64CFG2")) {
        n=sscanf(text,"NPS64CFG2 %u %u %u %u %n",&t,&w,&slot,&f,&end);
        if(n!=4 || !tail(text+end) || w>1)return false;
    } else if(!strcmp(format,"NPS64CFG1")) {
        n=sscanf(text,"NPS64CFG1 %u %u %u %n",&t,&w,&slot,&end);
        if(n!=3 || !tail(text+end) || w>1)return false;
    } else return false;
    if(t>2 || slot>7 || f>1 || pak>2)return false;
    /* Legacy global aspect must not turn every unconfigured game into 16:9. */
    *out=(Nps64AppSettings){.theme=t,.hud_mode=hud,.defaults={.filtered_vi=f!=0,.slot=slot,.controller_pak=pak}};return true;
}
bool nps64_settings_save(const char *path,const Nps64AppSettings *s) {
    if(s->theme>2 || s->hud_mode>2 || !valid(&s->defaults))return false;
    char text[96];int n=snprintf(text,sizeof(text),"NPS64CFG4 %u %u %u %u %u\n",s->theme,s->hud_mode,s->defaults.slot,s->defaults.filtered_vi,s->defaults.controller_pak);
    return n>0 && n<(int)sizeof(text) && file_write_atomic(path,text,(size_t)n);
}
static bool game_path(char path[PATH_CAP],const char *dir,const char *key) {
    if(!key || strlen(key)!=20 || strncmp(key,"rom-",4) || strspn(key+4,"0123456789abcdef")!=16)return false;
    int n=snprintf(path,PATH_CAP,"%s/%s.cfg",dir,key);return n>0 && n<PATH_CAP;
}
bool nps64_game_settings_load(const char *dir,const char *key,const Nps64GameSettings *defaults,Nps64GameSettings *out) {
    *out=*defaults;out->widescreen=false;
    char path[PATH_CAP],text[256];if(!game_path(path,dir,key) || !read_text(path,text))return false;
    unsigned w,f,slot,pak;int end=0;
    if(sscanf(text,"NPS64GAME1 %u %u %u %u %n",&w,&f,&slot,&pak,&end)!=4 || !tail(text+end) || w>1 || f>1 || slot>7 || pak>2)return false;
    *out=(Nps64GameSettings){w!=0,f!=0,slot,pak};return true;
}
bool nps64_game_settings_save(const char *dir,const char *key,const Nps64GameSettings *s) {
    char path[PATH_CAP],text[96];if(!valid(s) || !game_path(path,dir,key))return false;
    if(mkdir(dir,0777) && errno!=EEXIST)return false;
    int n=snprintf(text,sizeof(text),"NPS64GAME1 %u %u %u %u\n",s->widescreen,s->filtered_vi,s->slot,s->controller_pak);
    return n>0 && n<(int)sizeof(text) && file_write_atomic(path,text,(size_t)n);
}
bool nps64_game_settings_reset(const char *dir,const char *key) {
    char path[PATH_CAP];if(!game_path(path,dir,key))return false;
    return unlink(path)==0 || errno==ENOENT;
}
