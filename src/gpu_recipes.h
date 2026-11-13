#pragma once
#include "gpu_cache_store.h"
#ifdef __cplusplus
extern "C" {
#endif
enum { NPS64_RECIPE_CONSTANTS=12, NPS64_RECIPE_LIMIT=1024 };
typedef struct {uint64_t shader,pipeline;uint32_t mask,constants[NPS64_RECIPE_CONSTANTS];} Nps64GpuRecipe;
bool nps64_gpu_recipes_load(const char*,const Nps64GpuCacheIdentity*,Nps64GpuRecipe**,size_t*);
bool nps64_gpu_recipes_save(const char*,const Nps64GpuCacheIdentity*,const Nps64GpuRecipe*,size_t);
/* One frontend/device at a time. Called after retro_run returns, never in a
 * Vulkan callback or concurrently with device destruction. */
void nps64_gpu_recipes_bind(void*,unsigned(*)(void*));
void nps64_gpu_recipes_unbind(void*);
unsigned nps64_gpu_recipes_prepare(void);
#ifdef __cplusplus
}
#endif
