/* Optional local cache: bounded, device-specific, integrity-checked for accidental
 * corruption and replaced atomically. Never contains game/save data.
 * SPDX-License-Identifier: GPL-2.0-or-later */
#define _POSIX_C_SOURCE 200809L
#include "gpu_cache_store.h"
#include "storage.h"
#include "sha256.h"
#include "log.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>
enum { HEADER=84 };
static void put32(uint8_t *p,uint32_t x){for(unsigned i=0;i<4;i++)p[i]=(uint8_t)(x>>(8*i));}
static uint32_t get32(const uint8_t *p){return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;}
static int filename(char path[PATH_CAP],const char *dir,const Nps64GpuCacheIdentity *key) {
    if(!dir || !*dir)return 0;
    char uuid[33];for(unsigned i=0;i<16;i++)snprintf(uuid+2*i,3,"%02x",key->uuid[i]);
    int n=snprintf(path,PATH_CAP,"%s/%08x-%08x-%08x-%u-%s.bin",dir,key->vendor,key->device,key->driver,key->kind,uuid);
    return n>0 && n<PATH_CAP;
}
static void identity(uint8_t *header,const Nps64GpuCacheIdentity *key) {
    memcpy(header,"NPS64PC1",8);put32(header+8,1);put32(header+12,key->vendor);
    put32(header+16,key->device);put32(header+20,key->driver);put32(header+24,key->kind);memcpy(header+28,key->uuid,16);
}
bool nps64_gpu_cache_load(const char *dir,const Nps64GpuCacheIdentity *key,void **out,size_t *size) {
    *out=NULL;*size=0;char path[PATH_CAP];if(!filename(path,dir,key))return false;
    /* Read the bounded header first. Avoid stat/stdio size disagreement and
     * a second pathname lookup; never allocate from an unchecked file size. */
    FILE *file=fopen(path,"rb");
    if(!file){nps64_log("[pipeline-cache-store] miss reason=open errno=%d path=%s\n",errno,path);return false;}
    uint8_t header[HEADER],expected[44];identity(expected,key);
    size_t header_read=fread(header,1,HEADER,file);
    const char *reason=header_read!=HEADER?"short-header":NULL;
    if(!reason && memcmp(header,expected,44))reason="identity";
    bool ok=!reason;
    uint32_t length=ok?get32(header+44):0;
    if(ok && (!length || length>NPS64_GPU_CACHE_LIMIT)){ok=false;reason="length";}
    void *data=ok?malloc(length):NULL;
    if(ok && !data){ok=false;reason="allocation";}
    size_t payload_read=0;
    if(ok) {
        payload_read=fread(data,1,length,file);
        if(payload_read!=length){ok=false;reason="short-payload";}
        else {
            uint8_t extra;
            if(fread(&extra,1,1,file)){ok=false;reason="trailing-data";}
            else if(ferror(file)){ok=false;reason="read-error";}
        }
    }
    fclose(file);
    if(ok){
        Nps64Sha256 hash;uint8_t digest[32];nps64_sha256_init(&hash);nps64_sha256_update(&hash,data,length);nps64_sha256_final(&hash,digest);
        if(memcmp(header+52,digest,32)){ok=false;reason="checksum";}
        else if(get32(header+48)!=0){ok=false;reason="reserved-header";}
    }
    if(!ok){nps64_log("[pipeline-cache-store] miss reason=%s header=%zu payload=%zu expected=%u path=%s\n",reason,header_read,payload_read,length,path);free(data);return false;}
    nps64_log("[pipeline-cache-store] hit bytes=%u path=%s\n",length,path);
    *out=data;*size=length;return true;
}
bool nps64_gpu_cache_save(const char *dir,const Nps64GpuCacheIdentity *key,const void *data,size_t size) {
    if(!data || !size || size>NPS64_GPU_CACHE_LIMIT)return false;
    char path[PATH_CAP];if(!filename(path,dir,key))return false;
    if(mkdir(dir,0777) && errno!=EEXIST)return false;
    uint8_t *blob=calloc(1,HEADER+size);if(!blob)return false;
    identity(blob,key);put32(blob+44,(uint32_t)size);
    Nps64Sha256 hash;nps64_sha256_init(&hash);nps64_sha256_update(&hash,data,size);nps64_sha256_final(&hash,blob+52);
    memcpy(blob+HEADER,data,size);bool ok=file_write_atomic(path,blob,HEADER+size);free(blob);return ok;
}
