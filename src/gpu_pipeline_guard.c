/* Refuse high-pressure compute pipelines before the renderer can bind them.
 * This is not a GPU watchdog and does not validate memory/barrier correctness.
 * SPDX-License-Identifier: GPL-2.0-or-later */
#define VK_NO_PROTOTYPES
#include "gpu_pipeline_guard.h"
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "gpu_compile_worker.h"
#ifdef __ORBIS__
#include "log.h"
#include <orbis/libkernel.h>
#define REPORT nps64_log
#else
#define REPORT(...) fprintf(stderr,__VA_ARGS__)
#endif
#define DETAIL(...) do { if(getenv("NPS64_GPU_API_TRACE"))REPORT(__VA_ARGS__); } while(0)
static PFN_vkGetInstanceProcAddr original_gipa;
static PFN_vkGetDeviceProcAddr original_gdpa;
static PFN_vkCreateDevice original_create_device;
static int failed;
int nps64_pipeline_guard_failed(void){return __atomic_load_n(&failed,__ATOMIC_ACQUIRE);}
static VkResult reject(const char *reason) {
    __atomic_store_n(&failed,1,__ATOMIC_RELEASE);
    REPORT("[pipeline-guard] REJECT %s; compute pipeline unavailable to renderer\n",reason);
    return VK_ERROR_FEATURE_NOT_PRESENT;
}
static VKAPI_ATTR VkResult VKAPI_CALL guarded_create_device(VkPhysicalDevice gpu,
        const VkDeviceCreateInfo *info,const VkAllocationCallbacks *allocator,VkDevice *device) {
    if(!original_create_device || info->enabledExtensionCount>=128)return reject("device extension capacity");
    const char *extensions[128];unsigned count=info->enabledExtensionCount;int present=0;
    for(unsigned i=0;i<count;i++) {
        extensions[i]=info->ppEnabledExtensionNames[i];
        if(!strcmp(extensions[i],VK_KHR_PIPELINE_EXECUTABLE_PROPERTIES_EXTENSION_NAME))present=1;
    }
    if(!present)extensions[count++]=VK_KHR_PIPELINE_EXECUTABLE_PROPERTIES_EXTENSION_NAME;
    const VkBaseInStructure *chain=(const VkBaseInStructure*)info->pNext;
    int feature_present=0;
    for(unsigned n=0;chain && n<64;n++,chain=chain->pNext) {
        if(chain->sType==VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PIPELINE_EXECUTABLE_PROPERTIES_FEATURES_KHR) {
            const VkPhysicalDevicePipelineExecutablePropertiesFeaturesKHR *features=(const void*)chain;
            if(!features->pipelineExecutableInfo)return reject("executable statistics disabled");
            feature_present=1;
        }
    }
    if(chain)return reject("invalid device feature chain");
    VkPhysicalDevicePipelineExecutablePropertiesFeaturesKHR features={
        .sType=VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PIPELINE_EXECUTABLE_PROPERTIES_FEATURES_KHR,
        .pNext=(void*)info->pNext,.pipelineExecutableInfo=VK_TRUE};
    VkDeviceCreateInfo modified=*info;
    modified.enabledExtensionCount=count;modified.ppEnabledExtensionNames=extensions;
    if(!feature_present)modified.pNext=&features;
    VkResult result=original_create_device(gpu,&modified,allocator,device);
    if(result!=VK_SUCCESS)__atomic_store_n(&failed,1,__ATOMIC_RELEASE);
    return result;
}
static VKAPI_ATTR VkResult VKAPI_CALL guarded_create_compute(VkDevice device,VkPipelineCache cache,
        uint32_t count,const VkComputePipelineCreateInfo *infos,
        const VkAllocationCallbacks *allocator,VkPipeline *pipelines) {
    if(!count || count>16 || !original_gdpa)return reject("pipeline batch capacity");
    for(unsigned i=0;i<count;i++)pipelines[i]=VK_NULL_HANDLE;
    if(nps64_pipeline_guard_failed())return reject("previous pipeline rejection");
    PFN_vkCreateComputePipelines create=(PFN_vkCreateComputePipelines)original_gdpa(device,"vkCreateComputePipelines");
    PFN_vkDestroyPipeline destroy=(PFN_vkDestroyPipeline)original_gdpa(device,"vkDestroyPipeline");
    PFN_vkGetPipelineExecutablePropertiesKHR properties=(PFN_vkGetPipelineExecutablePropertiesKHR)original_gdpa(device,"vkGetPipelineExecutablePropertiesKHR");
    PFN_vkGetPipelineExecutableStatisticsKHR statistics=(PFN_vkGetPipelineExecutableStatisticsKHR)original_gdpa(device,"vkGetPipelineExecutableStatisticsKHR");
    if(!create || !destroy || !properties || !statistics)return reject("missing statistics API");
    /* Some renderer callbacks run on 64KiB stacks. Keep the bounded batch
     * and ~35KiB statistics table off that stack, including before worker entry. */
    VkComputePipelineCreateInfo *modified=calloc(count,sizeof(*modified));
    if(!modified)return reject("pipeline batch allocation failed");
    for(unsigned i=0;i<count;i++){modified[i]=infos[i];modified[i].flags|=VK_PIPELINE_CREATE_CAPTURE_STATISTICS_BIT_KHR;}
#ifdef __ORBIS__
    if(getenv("NPS64_GPU_API_TRACE")) {
    OrbisKernelVirtualQueryInfo mapping;memset(&mapping,0,sizeof(mapping));
    int stack_query=sceKernelVirtualQuery(&mapping,0,&mapping,sizeof(mapping));
    DETAIL("[pipeline-guard] stack query=%d marker=%p mapping=%p..%p is_stack=%u\n",
           stack_query,(void*)&mapping,mapping.unk01,mapping.unk02,mapping.isStack);
    }
#endif
    DETAIL("[pipeline-guard] driver-create begin count=%u cache=0x%llx\n",count,(unsigned long long)(uintptr_t)cache);
    VkResult result=getenv("NPS64_GPU_COMPILE_WORKER")?
        nps64_gpu_compile_worker(create,device,cache,count,modified,allocator,pipelines):
        create(device,cache,count,modified,allocator,pipelines);
    free(modified);
    DETAIL("[pipeline-guard] driver-create end result=%d\n",result);
    if(result!=VK_SUCCESS)return result;
    VkPipelineExecutableStatisticKHR *stats=calloc(64,sizeof(*stats));
    const char *reason=stats?NULL:"statistics allocation failed";
    for(unsigned i=0;i<count && !reason;i++) {
        VkPipelineInfoKHR pi={.sType=VK_STRUCTURE_TYPE_PIPELINE_INFO_KHR,.pipeline=pipelines[i]};
        unsigned executables=0;
        DETAIL("[pipeline-guard] properties begin\n");
        VkResult query=properties(device,&pi,&executables,NULL);
        DETAIL("[pipeline-guard] properties end result=%d count=%u\n",query,executables);
        if(query!=VK_SUCCESS || !executables || executables>16){reason="invalid executable count";break;}
        for(unsigned index=0;index<executables && !reason;index++) {
            VkPipelineExecutableInfoKHR ei={.sType=VK_STRUCTURE_TYPE_PIPELINE_EXECUTABLE_INFO_KHR,.pipeline=pipelines[i],.executableIndex=index};
            uint32_t n=0;
            DETAIL("[pipeline-guard] statistics-count begin executable=%u\n",index);
            query=statistics(device,&ei,&n,NULL);
            DETAIL("[pipeline-guard] statistics-count end result=%d count=%u\n",query,n);
            if(query!=VK_SUCCESS || !n || n>64){reason="invalid statistics count";break;}
            memset(stats,0,64*sizeof(*stats));
            for(unsigned k=0;k<n;k++)stats[k].sType=VK_STRUCTURE_TYPE_PIPELINE_EXECUTABLE_STATISTIC_KHR;
            DETAIL("[pipeline-guard] statistics-read begin\n");
            query=statistics(device,&ei,&n,stats);
            DETAIL("[pipeline-guard] statistics-read end result=%d count=%u\n",query,n);
            if(query!=VK_SUCCESS || n>64){reason="statistics query failed";break;}
            uint64_t sgprs=0,vgprs=0,scratch=0,spills=0,lds=0;unsigned seen=0;
            for(unsigned k=0;k<n;k++) {
                uint64_t value;
                if(stats[k].format==VK_PIPELINE_EXECUTABLE_STATISTIC_FORMAT_UINT64_KHR)value=stats[k].value.u64;
                else if(stats[k].format==VK_PIPELINE_EXECUTABLE_STATISTIC_FORMAT_INT64_KHR && stats[k].value.i64>=0)value=(uint64_t)stats[k].value.i64;
                else continue;
                if(!strcmp(stats[k].name,"SGPRs")){sgprs=value;seen|=1;}
                else if(!strcmp(stats[k].name,"VGPRs")){vgprs=value;seen|=2;}
                else if(!strcmp(stats[k].name,"Scratch size")){scratch=value;seen|=4;}
                else if(!strcmp(stats[k].name,"Spilled VGPRs")){spills=value;seen|=8;}
                else if(!strcmp(stats[k].name,"LDS size")){lds=value;seen|=16;}
            }
            REPORT("[pipeline-guard] sgprs=%llu vgprs=%llu scratch=%llu spilled_vgprs=%llu lds=%llu\n",
                (unsigned long long)sgprs,(unsigned long long)vgprs,(unsigned long long)scratch,(unsigned long long)spills,(unsigned long long)lds);
            if(seen!=31)reason="missing resource statistics";
            else if(sgprs>96 || vgprs>128 || lds>512 || scratch || spills)reason="compute resource limit";
        }
    }
    free(stats);
    if(reason) {
        for(unsigned i=0;i<count;i++)if(pipelines[i]){destroy(device,pipelines[i],allocator);pipelines[i]=VK_NULL_HANDLE;}
        return reject(reason);
    }
    return VK_SUCCESS;
}
static VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL guarded_gdpa(VkDevice device,const char *name) {
    if(!strcmp(name,"vkCreateComputePipelines"))return (PFN_vkVoidFunction)guarded_create_compute;
    return original_gdpa?original_gdpa(device,name):NULL;
}
static VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL guarded_gipa(VkInstance instance,const char *name) {
    if(!strcmp(name,"vkCreateDevice")) {
        original_create_device=(PFN_vkCreateDevice)original_gipa(instance,name);
        return original_create_device?(PFN_vkVoidFunction)guarded_create_device:NULL;
    }
    if(!strcmp(name,"vkGetDeviceProcAddr")) {
        original_gdpa=(PFN_vkGetDeviceProcAddr)original_gipa(instance,name);
        return original_gdpa?(PFN_vkVoidFunction)guarded_gdpa:NULL;
    }
    if(!strcmp(name,"vkCreateComputePipelines"))return (PFN_vkVoidFunction)guarded_create_compute;
    return original_gipa?original_gipa(instance,name):NULL;
}
PFN_vkGetInstanceProcAddr nps64_pipeline_guard_wrap(PFN_vkGetInstanceProcAddr loader) {
    original_gipa=loader;original_gdpa=NULL;original_create_device=NULL;
    __atomic_store_n(&failed,0,__ATOMIC_RELEASE);
    return loader?guarded_gipa:NULL;
}
