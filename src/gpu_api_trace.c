/* Vulkan entry/return checkpoints for the isolated PS4 diagnostic.
 * A missing return narrows the failure; it does not identify a GPU fault.
 * SPDX-License-Identifier: GPL-2.0-or-later */
#define VK_NO_PROTOTYPES
#include "gpu_api_trace.h"
#include "log.h"
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#ifdef NPS64_HOST_COMPILE_WORKER_TEST
#include "gpu_compile_worker.h"
#endif

#define APIS(X) \
    X(CreateComputePipelines) X(BeginCommandBuffer) X(EndCommandBuffer) \
    X(CmdBindPipeline) X(CmdBindDescriptorSets) X(CmdPushConstants) \
    X(CmdDispatch) X(CmdDispatchIndirect) X(QueueSubmit) X(QueueSubmit2) \
    X(QueueSubmit2KHR) X(WaitForFences)
#define DECLARE(name) static PFN_vk##name original_##name;
APIS(DECLARE)
#undef DECLARE
static PFN_vkGetInstanceProcAddr original_gipa;
static PFN_vkGetDeviceProcAddr original_gdpa;
static uint64_t sequence;
#define BEGIN(name,format,...) \
    int trace_enabled=getenv("NPS64_GPU_API_TRACE")!=NULL; \
    uint64_t id=trace_enabled?__atomic_add_fetch(&sequence,1,__ATOMIC_RELAXED):0; \
    if(trace_enabled)nps64_log("[gpu-api] id=%llu begin vk" #name " " format "\n", \
              (unsigned long long)id,__VA_ARGS__)
#define END(name) do { if(trace_enabled)nps64_log("[gpu-api] id=%llu end vk" #name "\n",(unsigned long long)id); } while(0)
#define RESULT(name) do { \
    if(trace_enabled)nps64_log("[gpu-api] id=%llu end vk" #name " result=%d\n",(unsigned long long)id,result); \
    else if(result<0)nps64_log("[gpu-api-error] vk" #name " result=%d\n",result); \
} while(0)

static VKAPI_ATTR VkResult VKAPI_CALL trace_CreateComputePipelines(VkDevice device,VkPipelineCache cache,
        uint32_t count,const VkComputePipelineCreateInfo *infos,const VkAllocationCallbacks *allocator,VkPipeline *pipelines) {
    BEGIN(CreateComputePipelines,"device=%p count=%u",(void*)device,count);
#ifdef NPS64_HOST_COMPILE_WORKER_TEST
    /* Native test adapter: Lavapipe lacks the PS4 resource statistics guard. */
    VkResult result=nps64_gpu_compile_worker(original_CreateComputePipelines,device,cache,count,infos,allocator,pipelines);
#else
    VkResult result=original_CreateComputePipelines(device,cache,count,infos,allocator,pipelines);
#endif
    RESULT(CreateComputePipelines);return result;
}
static VKAPI_ATTR VkResult VKAPI_CALL trace_BeginCommandBuffer(VkCommandBuffer command,const VkCommandBufferBeginInfo *info) {
    BEGIN(BeginCommandBuffer,"command=%p",(void*)command);
    VkResult result=original_BeginCommandBuffer(command,info);RESULT(BeginCommandBuffer);return result;
}
static VKAPI_ATTR VkResult VKAPI_CALL trace_EndCommandBuffer(VkCommandBuffer command) {
    BEGIN(EndCommandBuffer,"command=%p",(void*)command);
    VkResult result=original_EndCommandBuffer(command);RESULT(EndCommandBuffer);return result;
}
static VKAPI_ATTR void VKAPI_CALL trace_CmdBindPipeline(VkCommandBuffer command,VkPipelineBindPoint point,VkPipeline pipeline) {
    BEGIN(CmdBindPipeline,"command=%p point=%u pipeline=0x%llx",(void*)command,(unsigned)point,(unsigned long long)(uintptr_t)pipeline);
    original_CmdBindPipeline(command,point,pipeline);END(CmdBindPipeline);
}
static VKAPI_ATTR void VKAPI_CALL trace_CmdBindDescriptorSets(VkCommandBuffer command,VkPipelineBindPoint point,
        VkPipelineLayout layout,uint32_t first,uint32_t count,const VkDescriptorSet *sets,uint32_t dynamic_count,const uint32_t *offsets) {
    BEGIN(CmdBindDescriptorSets,"command=%p first=%u count=%u dynamic=%u",(void*)command,first,count,dynamic_count);
    original_CmdBindDescriptorSets(command,point,layout,first,count,sets,dynamic_count,offsets);END(CmdBindDescriptorSets);
}
static VKAPI_ATTR void VKAPI_CALL trace_CmdPushConstants(VkCommandBuffer command,VkPipelineLayout layout,
        VkShaderStageFlags stages,uint32_t offset,uint32_t size,const void *values) {
    BEGIN(CmdPushConstants,"command=%p offset=%u bytes=%u",(void*)command,offset,size);
    original_CmdPushConstants(command,layout,stages,offset,size,values);END(CmdPushConstants);
}
static VKAPI_ATTR void VKAPI_CALL trace_CmdDispatch(VkCommandBuffer command,uint32_t x,uint32_t y,uint32_t z) {
    BEGIN(CmdDispatch,"command=%p groups=%u,%u,%u",(void*)command,x,y,z);
    original_CmdDispatch(command,x,y,z);END(CmdDispatch);
}
static VKAPI_ATTR void VKAPI_CALL trace_CmdDispatchIndirect(VkCommandBuffer command,VkBuffer buffer,VkDeviceSize offset) {
    BEGIN(CmdDispatchIndirect,"command=%p buffer=0x%llx offset=%llu",(void*)command,
          (unsigned long long)(uintptr_t)buffer,(unsigned long long)offset);
    original_CmdDispatchIndirect(command,buffer,offset);END(CmdDispatchIndirect);
}
static VKAPI_ATTR VkResult VKAPI_CALL trace_QueueSubmit(VkQueue queue,uint32_t count,const VkSubmitInfo *infos,VkFence fence) {
    BEGIN(QueueSubmit,"queue=%p count=%u",(void*)queue,count);
    VkResult result=original_QueueSubmit(queue,count,infos,fence);RESULT(QueueSubmit);return result;
}
static VKAPI_ATTR VkResult VKAPI_CALL trace_QueueSubmit2(VkQueue queue,uint32_t count,const VkSubmitInfo2 *infos,VkFence fence) {
    BEGIN(QueueSubmit2,"queue=%p count=%u",(void*)queue,count);
    VkResult result=original_QueueSubmit2(queue,count,infos,fence);RESULT(QueueSubmit2);return result;
}
static VKAPI_ATTR VkResult VKAPI_CALL trace_QueueSubmit2KHR(VkQueue queue,uint32_t count,const VkSubmitInfo2 *infos,VkFence fence) {
    BEGIN(QueueSubmit2KHR,"queue=%p count=%u",(void*)queue,count);
    VkResult result=original_QueueSubmit2KHR(queue,count,infos,fence);RESULT(QueueSubmit2KHR);return result;
}
static VKAPI_ATTR VkResult VKAPI_CALL trace_WaitForFences(VkDevice device,uint32_t count,const VkFence *fences,VkBool32 all,uint64_t timeout) {
    BEGIN(WaitForFences,"device=%p count=%u timeout=%llu",(void*)device,count,(unsigned long long)timeout);
    VkResult result=original_WaitForFences(device,count,fences,all,timeout);RESULT(WaitForFences);return result;
}
static VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL trace_gdpa(VkDevice device,const char *name) {
    PFN_vkVoidFunction function=original_gdpa(device,name);
    if(!function)return NULL;
#define RESOLVE(api) if(!strcmp(name,"vk" #api)){original_##api=(PFN_vk##api)function;return (PFN_vkVoidFunction)trace_##api;}
    APIS(RESOLVE)
#undef RESOLVE
    return function;
}
static VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL trace_gipa(VkInstance instance,const char *name) {
    PFN_vkVoidFunction function=original_gipa(instance,name);
    if(function && !strcmp(name,"vkGetDeviceProcAddr")) {
        original_gdpa=(PFN_vkGetDeviceProcAddr)function;return (PFN_vkVoidFunction)trace_gdpa;
    }
    return function;
}
PFN_vkGetInstanceProcAddr nps64_gpu_api_trace_wrap(PFN_vkGetInstanceProcAddr loader) {
    original_gipa=loader;original_gdpa=NULL;sequence=0;
    return loader?trace_gipa:NULL;
}
