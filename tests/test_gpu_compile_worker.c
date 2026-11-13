#define _GNU_SOURCE
#define VK_NO_PROTOTYPES
#include "gpu_compile_worker.h"
#include "log.h"
#include <pthread.h>
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
static pthread_t caller;
static VkComputePipelineCreateInfo info;
static VkAllocationCallbacks allocator;
static unsigned calls;
static VkResult expected;
static VKAPI_ATTR VkResult VKAPI_CALL create(VkDevice device,VkPipelineCache cache,uint32_t count,
        const VkComputePipelineCreateInfo *infos,const VkAllocationCallbacks *callbacks,VkPipeline *pipelines) {
    if(info.flags&VK_PIPELINE_CREATE_FAIL_ON_PIPELINE_COMPILE_REQUIRED_BIT) {
        assert(pthread_equal(caller,pthread_self()));
    } else {
    assert(!pthread_equal(caller,pthread_self()));
    pthread_attr_t attr;assert(!pthread_getattr_np(pthread_self(),&attr));
    size_t stack;assert(!pthread_attr_getstacksize(&attr,&stack));assert(stack>=8u*1024u*1024u);
    pthread_attr_destroy(&attr);
    }
    assert((uintptr_t)device==0x100 && (uintptr_t)cache==0x200);
    assert(count==1 && infos==&info && callbacks==&allocator);
    pipelines[0]=expected==VK_SUCCESS?(VkPipeline)(uintptr_t)0x300:VK_NULL_HANDLE;
    calls++;return expected;
}
int main(void) {
    caller=pthread_self();info.sType=VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    VkComputePipelineCreateInfo saved=info;
    assert(nps64_log_init("build/test-gpu-compile-worker.log"));
    const VkResult results[]={VK_SUCCESS,VK_ERROR_OUT_OF_HOST_MEMORY,VK_ERROR_DEVICE_LOST};
    for(unsigned i=0;i<3;i++) {
        expected=results[i];VkPipeline pipeline=VK_NULL_HANDLE;
        VkResult result=nps64_gpu_compile_worker(create,(VkDevice)(uintptr_t)0x100,
            (VkPipelineCache)(uintptr_t)0x200,1,&info,&allocator,&pipeline);
        assert(result==expected && calls==i+1 && !memcmp(&info,&saved,sizeof(info)));
        assert(pipeline==(expected==VK_SUCCESS?(VkPipeline)(uintptr_t)0x300:VK_NULL_HANDLE));
    }
    info.flags=VK_PIPELINE_CREATE_FAIL_ON_PIPELINE_COMPILE_REQUIRED_BIT;
    const VkResult probes[]={VK_SUCCESS,VK_PIPELINE_COMPILE_REQUIRED};
    for(unsigned i=0;i<2;i++) {
        expected=probes[i];VkPipeline pipeline=VK_NULL_HANDLE;
        assert(nps64_gpu_compile_worker(create,(VkDevice)(uintptr_t)0x100,
            (VkPipelineCache)(uintptr_t)0x200,1,&info,&allocator,&pipeline)==expected);
        assert(calls==4+i && info.flags==VK_PIPELINE_CREATE_FAIL_ON_PIPELINE_COMPILE_REQUIRED_BIT);
    }
    nps64_log_close();puts("PASS compiler handoff: 8MiB worker, preserved arguments/errors; noncompiling cache probes stay on caller");
}
