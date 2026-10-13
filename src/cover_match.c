#include "cover_match.h"
#include <string.h>
#include <strings.h>
#include <stdio.h>
#include <ctype.h>
#include <stdint.h>
void cover_title(const char *rom,char *out,size_t cap) {
    if(!cap)return;size_t n=strcspn(rom,"([");const char *dot=strrchr(rom,'.');
    if(dot&&(size_t)(dot-rom)<n)n=(size_t)(dot-rom);
    while(n&&rom[n-1]==' ')n--;if(n>=cap)n=cap-1;memcpy(out,rom,n);out[n]=0;
    char *article=strstr(out,", The");if(article&&(article[5]==0||article[5]==' ')){memmove(article,article+5,strlen(article+5)+1);char tmp[256];snprintf(tmp,sizeof(tmp),"The %s",out);snprintf(out,cap,"%s",tmp);}
}
static void normalized(const char *name,char out[256]) {
    char title[256];cover_title(name,title,sizeof(title));char *edition=strstr(title," - Collector's Edition");if(edition)*edition=0;size_t n=0;
    for(size_t i=0;title[i]&&n<255;) {
        if(!isalnum((unsigned char)title[i])){i++;continue;}
        size_t start=i;while(isalnum((unsigned char)title[i]))i++;size_t len=i-start;
        if(len==3&&!strncasecmp(title+start,"the",3))continue;
        for(size_t k=start;k<i&&n<255;k++)out[n++]=(char)tolower((unsigned char)title[k]);
    }out[n]=0;
    if(!strcmp(out,"007goldeneye")||!strcmp(out,"goldeneye007"))snprintf(out,256,"goldeneye");
}
static int region(const char *name) {
    if(strstr(name,"(Europe)")||strstr(name,"(E)"))return 1;
    if(strstr(name,"(USA)")||strstr(name,"(U)"))return 2;
    if(strstr(name,"(Japan)")||strstr(name,"(J)"))return 3;return 0;
}
bool cover_match(const char *catalog,const char *rom,char *filename,size_t cap) {
    if(!catalog||!rom||!filename||!cap)return false;char target[256];normalized(rom,target);int wanted=region(rom),best=-1;
    for(const char *p=catalog;*p;) {
        const char *end=strchr(p,'\n');size_t n=end?(size_t)(end-p):strlen(p);while(n&&p[n-1]=='\r')n--;char line[256],key[256];
        if(n>0&&n<sizeof(line)&&n<cap){memcpy(line,p,n);line[n]=0;normalized(line,key);
            if(strstr(line,"(Debug)")||strstr(line,"(Beta")||strstr(line,"(Proto")){if(!end)break;p=end+1;continue;}
            if(!strcmp(target,key)){int r=region(line);int score=(r==wanted&&wanted?100:0)+(r==2?10:0)+(strstr(line,"(Rev")?0:2);
                if(score>best){best=score;snprintf(filename,cap,"%s",line);}}}
        if(!end)break;p=end+1;
    }return best>=0;
}
bool cover_url(const char *name,char *url,size_t cap) {
    const char *base="https://thumbnails.libretro.com/Nintendo%20-%20Nintendo%2064/Named_Boxarts/";
    size_t n=strlen(base);if(cap<=n)return false;memcpy(url,base,n);
    if(strchr(name,'/')||strchr(name,'\\')||strstr(name,".."))return false;
    const char hex[]="0123456789ABCDEF";
    for(const unsigned char *p=(const unsigned char*)name;*p;p++) {
        bool plain=isalnum(*p)||strchr("-._~",*p);size_t bytes=plain?1:3;if(n+bytes>=cap)return false;
        if(plain)url[n++]=(char)*p;else{url[n++]='%';url[n++]=hex[*p>>4];url[n++]=hex[*p&15];}
    }url[n]=0;return true;
}
void cover_key(const char *rom,char out[17]) {uint64_t h=UINT64_C(14695981039346656037);for(const unsigned char *p=(const unsigned char*)rom;*p;p++)h=(h^*p)*UINT64_C(1099511628211);snprintf(out,17,"%016llx",(unsigned long long)h);}
