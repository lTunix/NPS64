#ifndef NPS64_GPU_CACHE_STORE_H
#define NPS64_GPU_CACHE_STORE_H
#pragma once
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#ifdef __cplusplus
extern "C" {
#endif
enum { NPS64_GPU_CACHE_LIMIT=32*1024*1024 };
typedef struct {uint32_t vendor,device,driver,kind;uint8_t uuid[16];} Nps64GpuCacheIdentity;
bool nps64_gpu_cache_load(const char*,const Nps64GpuCacheIdentity*,void**,size_t*);
bool nps64_gpu_cache_save(const char*,const Nps64GpuCacheIdentity*,const void*,size_t);
#ifdef __cplusplus
}
#endif

#endif
