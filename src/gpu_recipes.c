/* Portable shader variant descriptions, never executable driver binaries.
 * SPDX-License-Identifier: GPL-2.0-or-later */
#include "gpu_recipes.h"
#include <stdlib.h>
#include <string.h>
enum { HEADER=16, RECORD=68 };
static void put(uint8_t *p,uint32_t x){for(unsigned i=0;i<4;i++)p[i]=(uint8_t)(x>>(8*i));}
static uint32_t get(const uint8_t *p){return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;}
static void put64(uint8_t*p,uint64_t x){put(p,(uint32_t)x);put(p+4,(uint32_t)(x>>32));}
static uint64_t get64(const uint8_t*p){return get(p)|(uint64_t)get(p+4)<<32;}
bool nps64_gpu_recipes_load(const char *dir,const Nps64GpuCacheIdentity *key,Nps64GpuRecipe **out,size_t *count) {
    *out=NULL;*count=0;void *data=NULL;size_t size=0;
    if(!nps64_gpu_cache_load(dir,key,&data,&size))return false;
    const uint8_t *p=data;bool ok=size>=HEADER && !memcmp(p,"NPS64RC1",8) && get(p+12)==0;
    uint32_t n=ok?get(p+8):0;
    ok=ok && n>0 && n<=NPS64_RECIPE_LIMIT && size==HEADER+(size_t)n*RECORD;
    Nps64GpuRecipe *r=ok?calloc(n,sizeof(*r)):NULL;if(!r)ok=false;
    for(uint32_t i=0;i<n && ok;i++) {
        const uint8_t *b=p+HEADER+i*RECORD;r[i].shader=get64(b);r[i].pipeline=get64(b+8);r[i].mask=get(b+16);
        if(r[i].mask>>NPS64_RECIPE_CONSTANTS){ok=false;break;}
        for(unsigned k=0;k<NPS64_RECIPE_CONSTANTS;k++) {
            r[i].constants[k]=get(b+20+k*4);
            if(!(r[i].mask&(1u<<k)) && r[i].constants[k])ok=false;
        }
        for(uint32_t j=0;j<i;j++)if(r[i].shader==r[j].shader && r[i].pipeline==r[j].pipeline)ok=false;
    }
    free(data);if(!ok){free(r);return false;}*out=r;*count=n;return true;
}
bool nps64_gpu_recipes_save(const char *dir,const Nps64GpuCacheIdentity *key,const Nps64GpuRecipe *r,size_t n) {
    if(!r || !n || n>NPS64_RECIPE_LIMIT)return false;
    uint8_t *p=calloc(1,HEADER+n*RECORD);if(!p)return false;
    memcpy(p,"NPS64RC1",8);put(p+8,(uint32_t)n);bool ok=true;
    for(size_t i=0;i<n && ok;i++) {
        uint8_t *b=p+HEADER+i*RECORD;
        if(r[i].mask>>NPS64_RECIPE_CONSTANTS){ok=false;break;}
        put64(b,r[i].shader);put64(b+8,r[i].pipeline);put(b+16,r[i].mask);
        for(unsigned k=0;k<NPS64_RECIPE_CONSTANTS;k++) {
            if(!(r[i].mask&(1u<<k)) && r[i].constants[k]){ok=false;break;}
            put(b+20+k*4,r[i].constants[k]);
        }
        for(size_t j=0;j<i;j++)if(r[i].shader==r[j].shader && r[i].pipeline==r[j].pipeline)ok=false;
    }
    if(ok)ok=nps64_gpu_cache_save(dir,key,p,HEADER+n*RECORD);free(p);return ok;
}
static void *active_device;
static unsigned (*prepare)(void*);
void nps64_gpu_recipes_bind(void *device,unsigned(*callback)(void*)){active_device=device;prepare=callback;}
void nps64_gpu_recipes_unbind(void *device){if(active_device==device){active_device=NULL;prepare=NULL;}}
unsigned nps64_gpu_recipes_prepare(void){unsigned(*f)(void*)=prepare;prepare=NULL;return f?f(active_device):0;}
