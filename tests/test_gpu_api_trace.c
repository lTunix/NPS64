#define _POSIX_C_SOURCE 200809L
#define VK_NO_PROTOTYPES
#include "gpu_api_trace.h"
#include "log.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
static unsigned called;
static VKAPI_ATTR VkResult VKAPI_CALL submit(VkQueue queue,uint32_t count,const VkSubmitInfo2 *infos,VkFence fence) {
    assert((uintptr_t)queue==0x100 && count==2 && (uintptr_t)infos==0x200 && (uintptr_t)fence==0x300);
    called++;return VK_ERROR_DEVICE_LOST;
}
static VKAPI_ATTR void VKAPI_CALL indirect(VkCommandBuffer command,VkBuffer buffer,VkDeviceSize offset) {
    assert((uintptr_t)command==0x400 && (uintptr_t)buffer==0x500 && offset==UINT64_C(0x100000004));called++;
}
static VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL gdpa(VkDevice device,const char *name) {
    assert((uintptr_t)device==0x600);
    if(!strcmp(name,"vkQueueSubmit2") || !strcmp(name,"vkQueueSubmit2KHR"))return (PFN_vkVoidFunction)submit;
    if(!strcmp(name,"vkCmdDispatchIndirect"))return (PFN_vkVoidFunction)indirect;
    return NULL;
}
static VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL gipa(VkInstance instance,const char *name) {
    assert((uintptr_t)instance==0x700);
    if(!strcmp(name,"vkGetDeviceProcAddr"))return (PFN_vkVoidFunction)gdpa;
    return NULL;
}
int main(void) {
    assert(!setenv("NPS64_GPU_API_TRACE","1",1));
    assert(!nps64_gpu_api_trace_wrap(NULL));
    assert(nps64_log_init("build/test-gpu-api-trace.log"));
    PFN_vkGetInstanceProcAddr wrapped=nps64_gpu_api_trace_wrap(gipa);
    VkInstance instance=(VkInstance)(uintptr_t)0x700;VkDevice device=(VkDevice)(uintptr_t)0x600;
    PFN_vkGetDeviceProcAddr resolve=(PFN_vkGetDeviceProcAddr)wrapped(instance,"vkGetDeviceProcAddr");
    assert(resolve && !resolve(device,"vkBeginCommandBuffer") && !wrapped(instance,"vkMissing"));
    const char *names[]={"vkQueueSubmit2","vkQueueSubmit2KHR"};
    for(unsigned i=0;i<2;i++) {
        PFN_vkQueueSubmit2 call=(PFN_vkQueueSubmit2)resolve(device,names[i]);assert(call);
        assert(call((VkQueue)(uintptr_t)0x100,2,(const VkSubmitInfo2*)(uintptr_t)0x200,(VkFence)(uintptr_t)0x300)==VK_ERROR_DEVICE_LOST);
    }
    PFN_vkCmdDispatchIndirect call=(PFN_vkCmdDispatchIndirect)resolve(device,"vkCmdDispatchIndirect");assert(call);
    call((VkCommandBuffer)(uintptr_t)0x400,(VkBuffer)(uintptr_t)0x500,UINT64_C(0x100000004));
    assert(called==3);
    struct stat before,after;assert(!stat("build/test-gpu-api-trace.log",&before));
    assert(!unsetenv("NPS64_GPU_API_TRACE"));
    call((VkCommandBuffer)(uintptr_t)0x400,(VkBuffer)(uintptr_t)0x500,UINT64_C(0x100000004));
    assert(!stat("build/test-gpu-api-trace.log",&after) && before.st_size==after.st_size && called==4);
    PFN_vkQueueSubmit2 failed=(PFN_vkQueueSubmit2)resolve(device,"vkQueueSubmit2");
    assert(failed((VkQueue)(uintptr_t)0x100,2,(const VkSubmitInfo2*)(uintptr_t)0x200,(VkFence)(uintptr_t)0x300)==VK_ERROR_DEVICE_LOST);
    assert(!stat("build/test-gpu-api-trace.log",&after) && after.st_size>before.st_size && called==5);
    nps64_log_close();
    puts("PASS Vulkan trace: unavailable APIs, submit aliases, unchanged arguments/64-bit offset and device-loss return");
    return 0;
}
