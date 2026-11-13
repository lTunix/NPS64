#define _POSIX_C_SOURCE 200809L
#include "gpu_cache_store.h"
#include <assert.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
static void rejected(const char *dir,const Nps64GpuCacheIdentity *key) {
    void *data=(void*)1;size_t size=99;
    assert(!nps64_gpu_cache_load(dir,key,&data,&size) && !data && !size);
}
int main(void) {
    char dir[]="build/test-gpu-cache-XXXXXX";assert(mkdtemp(dir));
    Nps64GpuCacheIdentity key={0x1002,0x9920,0x6800008,1,{0}};
    for(unsigned i=0;i<16;i++)key.uuid[i]=(uint8_t)i;
    unsigned char payload[257];for(unsigned i=0;i<sizeof(payload);i++)payload[i]=(uint8_t)(i*37);
    rejected(dir,&key);assert(nps64_gpu_cache_save(dir,&key,payload,sizeof(payload)));
    char path[512]={0};DIR *d=opendir(dir);assert(d);struct dirent *entry;
    while((entry=readdir(d)))if(strstr(entry->d_name,".bin"))snprintf(path,sizeof(path),"%s/%s",dir,entry->d_name);
    closedir(d);assert(*path);
    void *data=NULL;size_t size=0;assert(nps64_gpu_cache_load(dir,&key,&data,&size));
    assert(size==sizeof(payload) && !memcmp(data,payload,size));free(data);
    Nps64GpuCacheIdentity other=key;other.driver++;rejected(dir,&other);
    other=key;other.uuid[0]++;rejected(dir,&other);
    const unsigned offsets[]={0,8,12,16,20,24,28,44,48,52,84,340};
    for(unsigned i=0;i<sizeof(offsets)/sizeof(*offsets);i++) {
        FILE *f=fopen(path,"r+b");assert(f);assert(!fseek(f,offsets[i],SEEK_SET));int c=fgetc(f);assert(c!=EOF);
        assert(!fseek(f,offsets[i],SEEK_SET));assert(fputc(c^1,f)!=EOF);fclose(f);
        rejected(dir,&key);assert(nps64_gpu_cache_save(dir,&key,payload,sizeof(payload)));
    }
    assert(!truncate(path,10));rejected(dir,&key);assert(nps64_gpu_cache_save(dir,&key,payload,sizeof(payload)));
    FILE *f=fopen(path,"ab");assert(f);assert(fputc(1,f)!=EOF);fclose(f);rejected(dir,&key);
    assert(!truncate(path,(off_t)NPS64_GPU_CACHE_LIMIT+85));rejected(dir,&key);
    assert(nps64_gpu_cache_save(dir,&key,payload,sizeof(payload)));
    assert(!nps64_gpu_cache_save(dir,&key,NULL,0));
    assert(nps64_gpu_cache_load(dir,&key,&data,&size));free(data);
    char temp[520];snprintf(temp,sizeof(temp),"%s.tmp",path);f=fopen(temp,"wb");assert(f);fputs("interrupted",f);fclose(f);
    assert(nps64_gpu_cache_load(dir,&key,&data,&size));free(data);
    char longdir[1100];memset(longdir,'x',sizeof(longdir)-1);longdir[sizeof(longdir)-1]=0;
    assert(!nps64_gpu_cache_save(longdir,&key,payload,sizeof(payload)));
    unlink(temp);unlink(path);rmdir(dir);
    puts("PASS GPU cache store: identity/SHA256, truncation, trailing/oversized files, invalid writes, interrupted temporary file and path bounds");
}
