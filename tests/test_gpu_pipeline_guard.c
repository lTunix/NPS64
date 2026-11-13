#define VK_NO_PROTOTYPES
#include "gpu_pipeline_guard.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#ifdef NPS64_GUARD_ALLOC_TEST
#include <stdlib.h>
static int allocation_budget=-1;
void *__real_calloc(size_t,size_t);
void *__wrap_calloc(size_t count,size_t size) {
    if(allocation_budget==0)return NULL;
    if(allocation_budget>0)allocation_budget--;
    return __real_calloc(count,size);
}
#endif
#ifdef NPS64_GUARD_SMALL_STACK_TEST
#include <pthread.h>
#define main guard_suite
#endif
static uint64_t values[5]={72,48,0,0,512};
static unsigned destroyed,created;static int missing,query_failure;
static VKAPI_ATTR VkResult VKAPI_CALL create_device(VkPhysicalDevice gpu,const VkDeviceCreateInfo *info,
        const VkAllocationCallbacks *allocator,VkDevice *device) {
    (void)gpu;(void)allocator;
    assert(info->enabledExtensionCount==1);
    assert(!strcmp(info->ppEnabledExtensionNames[0],VK_KHR_PIPELINE_EXECUTABLE_PROPERTIES_EXTENSION_NAME));
    const VkPhysicalDevicePipelineExecutablePropertiesFeaturesKHR *f=info->pNext;
    assert(f && f->pipelineExecutableInfo);
    *device=(VkDevice)(uintptr_t)0x100;return VK_SUCCESS;
}
static VKAPI_ATTR VkResult VKAPI_CALL create_pipelines(VkDevice device,VkPipelineCache cache,uint32_t count,
        const VkComputePipelineCreateInfo *infos,const VkAllocationCallbacks *allocator,VkPipeline *pipelines) {
    (void)device;(void)cache;(void)allocator;assert(count==1);
    assert(infos[0].flags&VK_PIPELINE_CREATE_CAPTURE_STATISTICS_BIT_KHR);
    pipelines[0]=(VkPipeline)(uintptr_t)0x200;created++;return VK_SUCCESS;
}
static VKAPI_ATTR void VKAPI_CALL destroy_pipeline(VkDevice device,VkPipeline pipeline,const VkAllocationCallbacks *allocator) {
    (void)device;(void)allocator;assert(pipeline==(VkPipeline)(uintptr_t)0x200);destroyed++;
}
static VKAPI_ATTR VkResult VKAPI_CALL properties(VkDevice device,const VkPipelineInfoKHR *info,uint32_t *count,VkPipelineExecutablePropertiesKHR *p) {
    (void)device;(void)info;(void)p;*count=1;return VK_SUCCESS;
}
static VKAPI_ATTR VkResult VKAPI_CALL statistics(VkDevice device,const VkPipelineExecutableInfoKHR *info,uint32_t *count,VkPipelineExecutableStatisticKHR *stats) {
    (void)device;(void)info;if(query_failure)return VK_ERROR_UNKNOWN;
    unsigned n=missing?4:5;
    if(!stats){*count=n;return VK_SUCCESS;}
    assert(*count>=n);*count=n;
    const char *names[]={"SGPRs","VGPRs","Scratch size","Spilled VGPRs","LDS size"};
    for(unsigned i=0;i<n;i++){snprintf(stats[i].name,sizeof(stats[i].name),"%s",names[i]);stats[i].format=VK_PIPELINE_EXECUTABLE_STATISTIC_FORMAT_UINT64_KHR;stats[i].value.u64=values[i];}
    return VK_SUCCESS;
}
static VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL gdpa(VkDevice device,const char *name) {
    (void)device;
    if(!strcmp(name,"vkCreateComputePipelines"))return (PFN_vkVoidFunction)create_pipelines;
    if(!strcmp(name,"vkDestroyPipeline"))return (PFN_vkVoidFunction)destroy_pipeline;
    if(!strcmp(name,"vkGetPipelineExecutablePropertiesKHR"))return (PFN_vkVoidFunction)properties;
    if(!strcmp(name,"vkGetPipelineExecutableStatisticsKHR"))return (PFN_vkVoidFunction)statistics;
    return NULL;
}
static VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL gipa(VkInstance instance,const char *name) {
    (void)instance;
    if(!strcmp(name,"vkCreateDevice"))return (PFN_vkVoidFunction)create_device;
    if(!strcmp(name,"vkGetDeviceProcAddr"))return (PFN_vkVoidFunction)gdpa;
    return NULL;
}
static void run(int reject) {
    PFN_vkGetInstanceProcAddr loader=nps64_pipeline_guard_wrap(gipa);
    PFN_vkCreateDevice create=(PFN_vkCreateDevice)loader(VK_NULL_HANDLE,"vkCreateDevice");
    VkDeviceCreateInfo dci={.sType=VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};VkDevice device;
    assert(create(VK_NULL_HANDLE,&dci,NULL,&device)==VK_SUCCESS);
    PFN_vkGetDeviceProcAddr proc=(PFN_vkGetDeviceProcAddr)loader(VK_NULL_HANDLE,"vkGetDeviceProcAddr");
    PFN_vkCreateComputePipelines compile=(PFN_vkCreateComputePipelines)proc(device,"vkCreateComputePipelines");
    VkComputePipelineCreateInfo ci={.sType=VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO};VkPipeline pipeline;
    unsigned old_destroyed=destroyed;
    assert(compile(device,VK_NULL_HANDLE,1,&ci,NULL,&pipeline)==(reject?VK_ERROR_FEATURE_NOT_PRESENT:VK_SUCCESS));
    assert(nps64_pipeline_guard_failed()==reject);
    if(reject) {
        assert(pipeline==VK_NULL_HANDLE && destroyed==old_destroyed+1);
        unsigned old_created=created;
        assert(compile(device,VK_NULL_HANDLE,1,&ci,NULL,&pipeline)==VK_ERROR_FEATURE_NOT_PRESENT);
        assert(created==old_created && pipeline==VK_NULL_HANDLE);
    }else assert(pipeline==(VkPipeline)(uintptr_t)0x200);
}
int main(void) {
    run(0);
    values[0]=96;values[1]=128;run(0);
    values[0]=112;run(1);values[0]=72;
    values[1]=256;run(1);values[1]=48;
    values[2]=5120;run(1);values[2]=0;
    values[3]=1;run(1);values[3]=0;
    values[4]=1024;run(1);values[4]=512;
    missing=1;run(1);missing=0;
    query_failure=1;run(1);
#ifdef NPS64_GUARD_ALLOC_TEST
    query_failure=0;
    for(int budget=0;budget<2;budget++) {
        PFN_vkGetInstanceProcAddr loader=nps64_pipeline_guard_wrap(gipa);
        PFN_vkCreateDevice cd=(PFN_vkCreateDevice)loader(VK_NULL_HANDLE,"vkCreateDevice");
        VkDeviceCreateInfo dci={.sType=VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};VkDevice device;
        assert(cd(VK_NULL_HANDLE,&dci,NULL,&device)==VK_SUCCESS);
        PFN_vkGetDeviceProcAddr resolve=(PFN_vkGetDeviceProcAddr)loader(VK_NULL_HANDLE,"vkGetDeviceProcAddr");
        PFN_vkCreateComputePipelines compile=(PFN_vkCreateComputePipelines)resolve(device,"vkCreateComputePipelines");
        VkComputePipelineCreateInfo ci={.sType=VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO};
        VkPipeline pipeline=(VkPipeline)(uintptr_t)0xdead;
        unsigned before_created=created,before_destroyed=destroyed;
        allocation_budget=budget;
        assert(compile(device,VK_NULL_HANDLE,1,&ci,NULL,&pipeline)==VK_ERROR_FEATURE_NOT_PRESENT);
        allocation_budget=-1;
        assert(pipeline==VK_NULL_HANDLE && nps64_pipeline_guard_failed());
        assert(created==before_created+(unsigned)budget && destroyed==before_destroyed+(unsigned)budget);
        assert(compile(device,VK_NULL_HANDLE,1,&ci,NULL,&pipeline)==VK_ERROR_FEATURE_NOT_PRESENT);
        assert(created==before_created+(unsigned)budget);
    }
    puts("PASS allocation failures: before driver creation and after creation, destruction, NULL output, no retry");
#endif
    puts("PASS GPU resource guard: thresholds, scratch/spills/LDS rejection, missing stats, query failure, invalid-handle cleanup and no retry after rejection");
    return 0;
}
#ifdef NPS64_GUARD_SMALL_STACK_TEST
#undef main
static void *limited_stack(void *unused) {
    (void)unused;
    /* Model stack already consumed by the caller, before entering the guard. */
    volatile unsigned char caller_space[32768];
    for(unsigned i=0;i<sizeof(caller_space);i++)caller_space[i]=(unsigned char)i;
    int result=guard_suite();
    for(unsigned i=0;i<sizeof(caller_space);i++)assert(caller_space[i]==(unsigned char)i);
    return (void*)(uintptr_t)result;
}
int main(void) {
    pthread_attr_t attr;assert(!pthread_attr_init(&attr));
    assert(!pthread_attr_setstacksize(&attr,65536));
    pthread_t thread;assert(!pthread_create(&thread,&attr,limited_stack,NULL));
    pthread_attr_destroy(&attr);void *result=NULL;
    assert(!pthread_join(thread,&result));assert(!result);
    puts("PASS guard with 64KiB caller stack and 32KiB already occupied");
    return 0;
}
#endif
