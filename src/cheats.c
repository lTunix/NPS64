#include "cheats.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>
static bool hex_id(const char *s,unsigned n){return strlen(s)==n&&strspn(s,"0123456789abcdef")==n;}
bool nps64_cheat_code_valid(const char *s){
 unsigned count=0;if(!s||!*s)return false;
 while(*s){
  for(unsigned field=0;field<2;field++){
   unsigned n=field?4:8;for(unsigned i=0;i<n;i++)if(!s[i]||!strchr("0123456789ABCDEFabcdef",s[i]))return false;s+=n;
   if(!field){if(*s++!=' ')return false;}
  }
  if(++count>128)return false;if(*s){if(*s++!=' '||!*s)return false;}
 }
 return true;
}
void nps64_cheats_free(Nps64Cheats *c){free(c->items);*c=(Nps64Cheats){0};}
static bool identity(const char *path,char out[21]){
 unsigned char b[64];FILE*f=fopen(path,"rb");if(!f)return false;bool ok=fread(b,1,64,f)==64;fclose(f);if(!ok)return false;
 if(!memcmp(b,"\x37\x80\x40\x12",4)){for(unsigned i=0;i<64;i+=2){unsigned char t=b[i];b[i]=b[i+1];b[i+1]=t;}}
 else if(!memcmp(b,"\x40\x12\x37\x80",4)){for(unsigned i=0;i<64;i+=4){unsigned char a=b[i],v=b[i+1];b[i]=b[i+3];b[i+1]=b[i+2];b[i+2]=v;b[i+3]=a;}}
 if(memcmp(b,"\x80\x37\x12\x40",4))return false;
 snprintf(out,21,"%02X%02X%02X%02X-%02X%02X%02X%02X-%02X",b[16],b[17],b[18],b[19],b[20],b[21],b[22],b[23],b[62]);return true;
}
static bool config_path(char out[PATH_CAP],const char *dir,const char *key){
 if(strlen(key)!=20||strncmp(key,"rom-",4)||!hex_id(key+4,16))return false;
 int n=snprintf(out,PATH_CAP,"%s/%s.cfg",dir,key);return n>0&&n<PATH_CAP;
}
unsigned nps64_cheats_enabled(const Nps64Cheats*c){unsigned n=0;for(unsigned i=0;i<c->count;i++)n+=c->items[i].enabled;return n;}
static void load_selections(Nps64Cheats*c,const char*dir){
 char path[PATH_CAP],line[64],ids[NPS64_CHEAT_ACTIVE_MAX][17];unsigned count=0;
 if(!config_path(path,dir,c->key))return;FILE*f=fopen(path,"rb");if(!f)return;
 bool ok=fgets(line,sizeof line,f)&&!strcmp(line,"NPS64CHEATCFG1\n");
 while(ok&&fgets(line,sizeof line,f)){
  size_t n=strlen(line);if(!n||line[n-1]!='\n'||count==NPS64_CHEAT_ACTIVE_MAX){ok=false;break;}line[n-1]=0;
  if(!hex_id(line,16)){ok=false;break;}memcpy(ids[count++],line,17);
 }
 if(ferror(f))ok=false;fclose(f);if(!ok)return;
 for(unsigned k=0;k<count;k++)for(unsigned i=0;i<c->count;i++)if(!strcmp(c->items[i].id,ids[k])){
  for(unsigned j=0;j<c->count;j++)if(!strcmp(c->items[j].group,c->items[i].group))c->items[j].enabled=false;
  c->items[i].enabled=true;break;
 }
}
bool nps64_cheats_load(Nps64Cheats*c,const Game*g,const char*database,const char*dir){
 nps64_cheats_free(c);if(strlen(g->key)!=20||strncmp(g->key,"rom-",4)||!hex_id(g->key+4,16))return false;
 memcpy(c->key,g->key,21);if(!identity(g->path,c->identity))return false;
 FILE*f=fopen(database,"rb");if(!f)return false;char line[2048];bool matching=false,ok=true;
 if(!fgets(line,sizeof line,f)||strcmp(line,"NPS64CHEATS1\n")){fclose(f);return false;}
 unsigned capacity=0;size_t bytes=0;
 while(fgets(line,sizeof line,f)){
  size_t n=strlen(line);bytes+=n;if(bytes>8*1024*1024||!n||line[n-1]!='\n'){ok=false;break;}line[n-1]=0;
  if(!strncmp(line,"R|",2)){char*end=strchr(line+2,'|');if(!end){ok=false;break;}*end=0;matching=!strcmp(line+2,c->identity);continue;}
  if(!matching)continue;
  if(strncmp(line,"C|",2)){ok=false;break;}
  char*fields[4];char*cursor=line+2;
  for(unsigned i=0;i<4;i++){fields[i]=cursor;if(i<3){char*end=strchr(cursor,'|');if(!end){ok=false;break;}*end=0;cursor=end+1;}}
  if(!ok)break;
  if(!hex_id(fields[0],16)||!hex_id(fields[1],16)||!*fields[2]||strlen(fields[2])>=192||strlen(fields[3])>=NPS64_CHEAT_CODE_MAX||!nps64_cheat_code_valid(fields[3])){ok=false;break;}
  bool duplicate=false;for(unsigned i=0;i<c->count;i++)if(!strcmp(c->items[i].id,fields[0])){duplicate=true;break;}if(duplicate)continue;
  if(c->count==NPS64_CHEAT_MAX){c->truncated=true;continue;}
  if(c->count==capacity){unsigned next=capacity+64;void*p=realloc(c->items,next*sizeof(*c->items));if(!p){ok=false;break;}c->items=p;capacity=next;}
  Nps64Cheat*item=&c->items[c->count++];*item=(Nps64Cheat){0};strcpy(item->id,fields[0]);strcpy(item->group,fields[1]);strcpy(item->name,fields[2]);strcpy(item->code,fields[3]);
 }
 if(ferror(f))ok=false;fclose(f);if(!ok){nps64_cheats_free(c);return false;}load_selections(c,dir);return true;
}
static bool save(const Nps64Cheats*c,const char*dir){
 char path[PATH_CAP],text[1024]="NPS64CHEATCFG1\n";size_t used=strlen(text);
 if(!config_path(path,dir,c->key))return false;
 for(unsigned i=0;i<c->count;i++)if(c->items[i].enabled){if(used+18>=sizeof text)return false;memcpy(text+used,c->items[i].id,16);used+=16;text[used++]='\n';}
 if(mkdir(dir,0777)&&errno!=EEXIST)return false;return file_write_atomic(path,text,used);
}
bool nps64_cheats_toggle(Nps64Cheats*c,unsigned index,const char*dir){
 if(index>=c->count)return false;bool before[NPS64_CHEAT_MAX];for(unsigned i=0;i<c->count;i++)before[i]=c->items[i].enabled;
 bool on=!c->items[index].enabled;if(on)for(unsigned i=0;i<c->count;i++)if(!strcmp(c->items[i].group,c->items[index].group))c->items[i].enabled=false;
 c->items[index].enabled=on;
 if(nps64_cheats_enabled(c)<=NPS64_CHEAT_ACTIVE_MAX&&save(c,dir))return true;
 for(unsigned i=0;i<c->count;i++)c->items[i].enabled=before[i];return false;
}
bool nps64_cheats_disable_all(Nps64Cheats*c,const char*dir){
 bool before[NPS64_CHEAT_MAX];for(unsigned i=0;i<c->count;i++){before[i]=c->items[i].enabled;c->items[i].enabled=false;}
 if(save(c,dir))return true;for(unsigned i=0;i<c->count;i++)c->items[i].enabled=before[i];return false;
}
void nps64_cheats_apply(Nps64Cheats*c,bool reset,void(*reset_fn)(void),void(*set_fn)(unsigned,bool,const char*)){
 if(reset){reset_fn();for(unsigned i=0;i<c->count;i++)c->items[i].applied=false;}
 /* Disable without rebuilding codes, preserving the core's restoration state. */
 for(unsigned i=0;i<c->count;i++)if(c->items[i].applied&&!c->items[i].enabled){set_fn(i,false,NULL);c->items[i].applied=false;}
 for(unsigned i=0;i<c->count;i++)if(!c->items[i].applied&&c->items[i].enabled){set_fn(i,true,c->items[i].code);c->items[i].applied=true;}
}
