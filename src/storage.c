#include "storage.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <ctype.h>

static uint32_t checksum(const unsigned char *p,size_t n) {
    uint32_t h=2166136261u;
    for(size_t i=0;i<n;i++) h=(h^p[i])*16777619u;
    return h;
}
bool rom_info(const char *path,Game *game) {
    unsigned char h[64], b[64]; struct stat st;
    if(strlen(path)>=sizeof(game->path) || stat(path,&st) || !S_ISREG(st.st_mode) || st.st_size<4096 || st.st_size>64*1024*1024) return false;
    FILE *f=fopen(path,"rb"); if(!f) return false;
    bool ok=fread(b,1,64,f)==64; fclose(f); if(!ok) return false;
    int mode;
    if(!memcmp(b,"\x80\x37\x12\x40",4)) mode=0;
    else if(!memcmp(b,"\x37\x80\x40\x12",4)) mode=1;
    else if(!memcmp(b,"\x40\x12\x37\x80",4)) mode=3;
    else return false;
    if(st.st_size % 4) return false;
    for(int i=0;i<64;i++) h[i]=b[i^mode];
    memset(game,0,sizeof(*game)); strcpy(game->path,path);
    const char *base=strrchr(path,'/'); base=base?base+1:path;
    snprintf(game->name,sizeof(game->name),"%s",base);
    for(char *p=game->name;*p;p++) if((unsigned char)*p<32 || (unsigned char)*p>126) *p='?';
    snprintf(game->key,sizeof(game->key),"%02x%02x%02x%02x-%02x%02x%02x%02x-%02x",h[16],h[17],h[18],h[19],h[20],h[21],h[22],h[23],h[62]);
    return true;
}
bool rom_content_key(Game *game) {
    FILE *f=fopen(game->path,"rb"); if(!f) return false;
    /* Keep the main-thread stack bounded on console during ROM identification. */
    unsigned char buf[4096]; size_t n; uint64_t hash=UINT64_C(14695981039346656037); int mode=-1;
    while((n=fread(buf,1,sizeof(buf),f))) {
        if(n%4) {fclose(f);return false;}
        if(mode<0) {
            if(!memcmp(buf,"\x80\x37\x12\x40",4)) mode=0;
            else if(!memcmp(buf,"\x37\x80\x40\x12",4)) mode=1;
            else if(!memcmp(buf,"\x40\x12\x37\x80",4)) mode=3;
            else {fclose(f);return false;}
        }
        for(size_t i=0;i<n;i++) hash=(hash^buf[i^mode])*UINT64_C(1099511628211);
    }
    bool ok=!ferror(f) && mode>=0; fclose(f);
    if(ok) snprintf(game->key,sizeof(game->key),"rom-%016llx",(unsigned long long)hash);
    return ok;
}
static int compare_games(const void *a,const void *b) { return strcasecmp(((const Game*)a)->name,((const Game*)b)->name); }
size_t library_scan(const char *dir,Game *games,size_t capacity) {
    DIR *d=opendir(dir); if(!d) return 0;
    struct dirent *e; size_t n=0; char path[PATH_CAP];
    while(n<capacity && (e=readdir(d))) {
        const char *ext=strrchr(e->d_name,'.');
        if(!ext || (strcasecmp(ext,".z64") && strcasecmp(ext,".v64") && strcasecmp(ext,".n64"))) continue;
        int len=snprintf(path,sizeof(path),"%s/%s",dir,e->d_name);
        if(len<0 || (size_t)len>=sizeof(path)) continue;
        if(rom_info(path,&games[n])) n++;
    }
    closedir(d); qsort(games,n,sizeof(*games),compare_games); return n;
}
bool file_write_atomic(const char *path,const void *data,size_t size) {
    char tmp[PATH_CAP]; int n=snprintf(tmp,sizeof(tmp),"%s.tmp",path);
    if(n<0 || (size_t)n>=sizeof(tmp)) return false;
    FILE *f=fopen(tmp,"wb"); if(!f) return false;
    bool ok=fwrite(data,1,size,f)==size;
    if(fflush(f) || fsync(fileno(f))) ok=false;
    if(fclose(f)) ok=false;
    if(ok && rename(tmp,path)==0) return true;
    unlink(tmp); return false;
}
bool file_read_exact(const char *path,void *data,size_t size) {
    struct stat st; if(stat(path,&st) || (uint64_t)st.st_size!=size) return false;
    FILE *f=fopen(path,"rb"); if(!f) return false;
    bool ok=fread(data,1,size,f)==size; fclose(f); return ok;
}
/* Header binds the state to this core revision, ROM and exact payload size. */
typedef struct { char magic[8],core[41],key[40]; uint64_t size; uint32_t hash; } StateHeader;
static const char core_revision[]="890323d083c7f5719bd59ebd010d75b3b7f8d10f";
bool state_write(const char *path,const char *key,const void *data,size_t size) {
    if(size>64*1024*1024) return false;
    unsigned char *buf=calloc(1,sizeof(StateHeader)+size); if(!buf) return false;
    StateHeader *h=(StateHeader*)buf;
    memcpy(h->magic,"NPS64S01",8); strcpy(h->core,core_revision); snprintf(h->key,sizeof(h->key),"%s",key);
    h->size=size; h->hash=checksum(data,size); memcpy(buf+sizeof(*h),data,size);
    bool ok=file_write_atomic(path,buf,sizeof(*h)+size); free(buf); return ok;
}
bool state_read(const char *path,const char *key,void *data,size_t size) {
    if(size>64*1024*1024) return false;
    unsigned char *buf=malloc(sizeof(StateHeader)+size); if(!buf) return false;
    bool ok=file_read_exact(path,buf,sizeof(StateHeader)+size);
    if(ok) {
        StateHeader *h=(StateHeader*)buf;
        ok=!memcmp(h->magic,"NPS64S01",8) && !memcmp(h->core,core_revision,sizeof(core_revision)) && h->size==size && !strncmp(h->key,key,sizeof(h->key)) && h->hash==checksum(buf+sizeof(*h),size);
        if(ok) memcpy(data,buf+sizeof(*h),size);
    }
    free(buf); return ok;
}
