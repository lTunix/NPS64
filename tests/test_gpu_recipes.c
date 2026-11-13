#define _POSIX_C_SOURCE 200809L
#include "gpu_recipes.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <dirent.h>
#include <unistd.h>
static unsigned invoke(void *p){return ++*(unsigned*)p;}
static void rejected(const char *dir,const Nps64GpuCacheIdentity *key){Nps64GpuRecipe *out=(void*)1;size_t n=1;assert(!nps64_gpu_recipes_load(dir,key,&out,&n) && !out && !n);}
int main(void) {
    char dir[]="build/test-recipes-XXXXXX";assert(mkdtemp(dir));
    Nps64GpuCacheIdentity key={0x1002,0x9920,1,3,{0}};
    Nps64GpuRecipe r[2]={{0}};r[0].shader=0x123;r[0].pipeline=0xabc;r[0].mask=(1u<<0)|(1u<<3)|(1u<<11);
    r[0].constants[0]=8;r[0].constants[3]=17;r[0].constants[11]=9;r[1].shader=0x456;r[1].pipeline=0xdef;
    rejected(dir,&key);assert(nps64_gpu_recipes_save(dir,&key,r,2));
    Nps64GpuRecipe *out=NULL;size_t n=0;assert(nps64_gpu_recipes_load(dir,&key,&out,&n));assert(n==2 && !memcmp(r,out,sizeof(r)));free(out);
    assert(!nps64_gpu_recipes_save(dir,&key,r,0));assert(!nps64_gpu_recipes_save(dir,&key,r,NPS64_RECIPE_LIMIT+1));
    r[1].mask=1u<<12;assert(!nps64_gpu_recipes_save(dir,&key,r,2));r[1].mask=0;
    r[1].constants[2]=5;assert(!nps64_gpu_recipes_save(dir,&key,r,2));r[1].constants[2]=0;
    r[1]=r[0];assert(!nps64_gpu_recipes_save(dir,&key,r,2));r[1]=(Nps64GpuRecipe){.shader=0x456,.pipeline=0xdef};
    void *blob=NULL;size_t size;assert(nps64_gpu_cache_load(dir,&key,&blob,&size));
    unsigned char *good=malloc(size);assert(good);memcpy(good,blob,size);
    const unsigned positions[]={0,8,12,33,40};
    for(unsigned i=0;i<sizeof(positions)/sizeof(*positions);i++) {
        memcpy(blob,good,size);((unsigned char*)blob)[positions[i]]^=0xff;
        assert(nps64_gpu_cache_save(dir,&key,blob,size));rejected(dir,&key);
    }
    assert(nps64_gpu_cache_save(dir,&key,good,size-1));rejected(dir,&key);
    assert(nps64_gpu_recipes_save(dir,&key,r,2));
    Nps64GpuCacheIdentity binary=key;binary.kind=1;rejected(dir,&binary);
    unsigned a=0,b=5;nps64_gpu_recipes_bind(&a,invoke);nps64_gpu_recipes_unbind(&b);
    assert(nps64_gpu_recipes_prepare()==1 && nps64_gpu_recipes_prepare()==0);
    nps64_gpu_recipes_bind(&a,invoke);nps64_gpu_recipes_bind(&b,invoke);nps64_gpu_recipes_unbind(&a);
    assert(nps64_gpu_recipes_prepare()==6);nps64_gpu_recipes_unbind(&b);
    free(good);free(blob);
    DIR *d=opendir(dir);assert(d);struct dirent *e;while((e=readdir(d)))if(e->d_name[0]!='.'){char path[512];snprintf(path,sizeof(path),"%s/%s",dir,e->d_name);unlink(path);}closedir(d);rmdir(dir);
    puts("PASS recipes: portable roundtrip, schema/mask/constant/duplicate/count checks, corruption, kind isolation and one-shot lifecycle");
}
