/* Diagnostic compiler stack isolation. Keeps shader, cache and arguments intact.
 * SPDX-License-Identifier: GPL-2.0-or-later */
#define VK_NO_PROTOTYPES
#include "gpu_compile_worker.h"
#include "log.h"
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#ifdef __ORBIS__
#include <orbis/libkernel.h>
#endif
#define DETAIL(...) do { if(getenv("NPS64_GPU_API_TRACE"))nps64_log(__VA_ARGS__); } while(0)
typedef struct {
    PFN_vkCreateComputePipelines create;
    VkDevice device;VkPipelineCache cache;uint32_t count;
    const VkComputePipelineCreateInfo *infos;
    const VkAllocationCallbacks *allocator;VkPipeline *pipelines;
    VkResult result;
} CompileCall;
static void *compile_thread(void *data) {
    CompileCall *call=data;
#ifdef __ORBIS__
    if(getenv("NPS64_GPU_API_TRACE")) {
    OrbisKernelVirtualQueryInfo mapping;memset(&mapping,0,sizeof(mapping));
    int result=sceKernelVirtualQuery(&mapping,0,&mapping,sizeof(mapping));
    DETAIL("[compiler-worker] stack query=%d marker=%p mapping=%p..%p is_stack=%u\n",
              result,(void*)&mapping,mapping.unk01,mapping.unk02,mapping.isStack);
    }
#endif
    DETAIL("[compiler-worker] driver begin\n");
    call->result=call->create(call->device,call->cache,call->count,call->infos,call->allocator,call->pipelines);
    DETAIL("[compiler-worker] driver end result=%d\n",call->result);
    return NULL;
}
VkResult nps64_gpu_compile_worker(PFN_vkCreateComputePipelines create,VkDevice device,VkPipelineCache cache,
        uint32_t count,const VkComputePipelineCreateInfo *infos,const VkAllocationCallbacks *allocator,VkPipeline *pipelines) {
    /* Vulkan guarantees these cache probes will not compile. Keep the hot
     * nonblocking lookup on the caller; real compilation still gets 8MiB. */
    int probe=count>0;
    for(uint32_t i=0;i<count;i++)if(!(infos[i].flags&VK_PIPELINE_CREATE_FAIL_ON_PIPELINE_COMPILE_REQUIRED_BIT))probe=0;
    if(probe)return create(device,cache,count,infos,allocator,pipelines);
    pthread_attr_t attr;
    int error=pthread_attr_init(&attr);
    if(error){nps64_log("[compiler-worker] attr init error=%d\n",error);return VK_ERROR_INITIALIZATION_FAILED;}
    error=pthread_attr_setstacksize(&attr,8u*1024u*1024u);
    if(error){pthread_attr_destroy(&attr);nps64_log("[compiler-worker] stack attr error=%d\n",error);return VK_ERROR_INITIALIZATION_FAILED;}
    CompileCall call={create,device,cache,count,infos,allocator,pipelines,VK_ERROR_INITIALIZATION_FAILED};
    pthread_t thread;
    DETAIL("[compiler-worker] launch stack_bytes=8388608\n");
    error=pthread_create(&thread,&attr,compile_thread,&call);
    pthread_attr_destroy(&attr);
    if(error){nps64_log("[compiler-worker] create error=%d\n",error);return VK_ERROR_INITIALIZATION_FAILED;}
    error=pthread_join(thread,NULL);
    if(error){
        /* Returning while the worker uses this stack frame would be unsafe. */
        nps64_log("[compiler-worker] fatal join error=%d\n",error);abort();
    }
    nps64_log("[compiler-worker] joined result=%d\n",call.result);
    return call.result;
}
