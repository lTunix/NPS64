/* Compile a captured pipeline on a virtual BONAIRE device. No queue submission.
 * This inspects compiler output, never validates GPU execution or GNM. */
#ifdef __ORBIS__
#define VK_NO_PROTOTYPES
#endif
#include <vulkan/vulkan.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef NPS64_PIPELINE_CACHE_TEST
#include "gpu_cache_store.h"
#include <time.h>
#endif
#ifdef NPS64_PIPELINE_GUARD_TEST
#include "gpu_pipeline_guard.h"
#endif
#ifdef __ORBIS__
#include "log.h"
#include "pipeline_compile_dispatch.h"
#define printf nps64_log
#define fprintf(stream,...) nps64_log(__VA_ARGS__)
#endif
#define CHECK(call) do {VkResult result=(call);if(result!=VK_SUCCESS){fprintf(stderr,"%s: %d\n",#call,result);return 1;}}while(0)
typedef struct {uint32_t magic,version,push_offset,push_size,push_stages,flags,bindings,specs,bytes;} Header;
int nps64_compile_captured_pipeline(const char *path) {
    FILE *f=fopen(path,"rb");if(!f)return 2;
    Header h;
    if(fread(&h,1,sizeof(h),f)!=sizeof(h) || h.magic!=0x5043504e || h.version!=1 ||
       h.bindings>128 || h.specs>64 || !h.bytes || h.bytes>16*1024*1024 || h.bytes%4 || h.flags || h.push_size>128)return 2;
    uint32_t bindings[128][5],specializations[64][2];
    if(fread(bindings,20,h.bindings,f)!=h.bindings || fread(specializations,8,h.specs,f)!=h.specs)return 2;
    uint32_t *code=malloc(h.bytes);if(!code || fread(code,1,h.bytes,f)!=h.bytes || code[0]!=0x07230203 || fgetc(f)!=EOF)return 2;
    fclose(f);
#ifdef __ORBIS__
    if(!probe_load_global())return 1;
#endif
    VkApplicationInfo app={.sType=VK_STRUCTURE_TYPE_APPLICATION_INFO,.apiVersion=VK_API_VERSION_1_2};
    VkInstanceCreateInfo ici={.sType=VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,.pApplicationInfo=&app};
    VkInstance instance;CHECK(vkCreateInstance(&ici,NULL,&instance));
#ifdef __ORBIS__
    if(!probe_load_instance(instance))return 1;
#endif
    uint32_t count=0;CHECK(vkEnumeratePhysicalDevices(instance,&count,NULL));if(!count)return 2;
    VkPhysicalDevice *devices=calloc(count,sizeof(*devices));if(!devices)return 2;
    CHECK(vkEnumeratePhysicalDevices(instance,&count,devices));VkPhysicalDevice gpu=devices[0];free(devices);
    VkPhysicalDeviceProperties props;vkGetPhysicalDeviceProperties(gpu,&props);
#ifdef __ORBIS__
    if(!strstr(props.deviceName,"Liverpool"))return 2;
#else
    if(!strstr(props.deviceName,"BONAIRE")){fprintf(stderr,"Expected virtual BONAIRE, got %s\n",props.deviceName);return 2;}
#endif
    printf("COMPILE ONLY device=%s driver=%u; no queue submission\n",props.deviceName,props.driverVersion);
    VkPhysicalDevicePipelineExecutablePropertiesFeaturesKHR executables={.sType=VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PIPELINE_EXECUTABLE_PROPERTIES_FEATURES_KHR};
    VkPhysicalDeviceVulkan11Features f11={.sType=VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES};
    f11.pNext=&executables;
    VkPhysicalDeviceVulkan12Features f12={.sType=VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES,.pNext=&f11};
    VkPhysicalDeviceFeatures2 features={.sType=VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,.pNext=&f12};
    vkGetPhysicalDeviceFeatures2(gpu,&features);
    if(!executables.pipelineExecutableInfo)return 2;
    /* The frontend passes a zeroed required-features structure and Granite
     * does not enable robustBufferAccess. Match that shader compilation mode. */
    features.features.robustBufferAccess=VK_FALSE;
    vkGetPhysicalDeviceQueueFamilyProperties(gpu,&count,NULL);
    VkQueueFamilyProperties *families=calloc(count,sizeof(*families));if(!families)return 2;
    vkGetPhysicalDeviceQueueFamilyProperties(gpu,&count,families);
    uint32_t family=0;while(family<count && !(families[family].queueFlags&VK_QUEUE_COMPUTE_BIT))family++;
    free(families);if(family==count)return 2;
    float priority=1;
    VkDeviceQueueCreateInfo qci={.sType=VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,.queueFamilyIndex=family,.queueCount=1,.pQueuePriorities=&priority};
    const char *extensions[]={VK_KHR_PIPELINE_EXECUTABLE_PROPERTIES_EXTENSION_NAME};
    VkDeviceCreateInfo dci={.sType=VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,.pNext=&f12,.pEnabledFeatures=&features.features,.queueCreateInfoCount=1,.pQueueCreateInfos=&qci,.enabledExtensionCount=1,.ppEnabledExtensionNames=extensions};
    VkDevice device;
#ifdef NPS64_PIPELINE_GUARD_TEST
#ifdef __ORBIS__
    PFN_vkGetInstanceProcAddr loader=vk_icdGetInstanceProcAddr;
#else
    PFN_vkGetInstanceProcAddr loader=vkGetInstanceProcAddr;
#endif
    const char *guard_mode=getenv("NPS64_COMPILE_WITH_GUARD");
    int guard_enabled=!guard_mode || strcmp(guard_mode,"0");
    printf("[pipeline-compile] guard=%d\n",guard_enabled);
    PFN_vkGetInstanceProcAddr guarded=guard_enabled?nps64_pipeline_guard_wrap(loader):loader;
    PFN_vkCreateDevice create_device=(PFN_vkCreateDevice)guarded(instance,"vkCreateDevice");
    CHECK(create_device(gpu,&dci,NULL,&device));
    PFN_vkGetDeviceProcAddr device_proc=(PFN_vkGetDeviceProcAddr)guarded(instance,"vkGetDeviceProcAddr");
#else
    CHECK(vkCreateDevice(gpu,&dci,NULL,&device));
#endif
#ifdef __ORBIS__
    if(!probe_load_device(device))return 1;
#endif
    VkPipelineCache pipeline_cache=VK_NULL_HANDLE;
#ifdef NPS64_PIPELINE_CACHE_TEST
    const char *cache_dir=getenv("NPS64_PIPELINE_TEST_CACHE_DIR");if(!cache_dir)return 2;
    Nps64GpuCacheIdentity identity={props.vendorID,props.deviceID,props.driverVersion,2,{0}};
    memcpy(identity.uuid,props.pipelineCacheUUID,16);
    void *cached=NULL;size_t cached_bytes=0;
    int loaded=nps64_gpu_cache_load(cache_dir,&identity,&cached,&cached_bytes);
    VkPipelineCacheCreateInfo cache_info={.sType=VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO,
        .initialDataSize=cached_bytes,.pInitialData=cached};
    CHECK(vkCreatePipelineCache(device,&cache_info,NULL,&pipeline_cache));free(cached);
    printf("CACHE loaded=%d bytes=%zu\n",loaded,cached_bytes);
#endif
    PFN_vkGetPipelineExecutablePropertiesKHR get_properties=(PFN_vkGetPipelineExecutablePropertiesKHR)vkGetDeviceProcAddr(device,"vkGetPipelineExecutablePropertiesKHR");
    PFN_vkGetPipelineExecutableStatisticsKHR get_statistics=(PFN_vkGetPipelineExecutableStatisticsKHR)vkGetDeviceProcAddr(device,"vkGetPipelineExecutableStatisticsKHR");
    if(!get_properties || !get_statistics)return 2;
    VkDescriptorSetLayout sets[4];unsigned set_count=0;
    for(unsigned i=0;i<h.bindings;i++){
        if(bindings[i][0]>=4 || bindings[i][1]>=32 || !bindings[i][3] || bindings[i][3]>32)return 2;
        if(bindings[i][0]+1>set_count)set_count=bindings[i][0]+1;
    }
    for(unsigned set=0;set<set_count;set++) {
        VkDescriptorSetLayoutBinding entries[128];unsigned n=0;
        for(unsigned i=0;i<h.bindings;i++)if(bindings[i][0]==set)
            entries[n++]=(VkDescriptorSetLayoutBinding){.binding=bindings[i][1],.descriptorType=bindings[i][2],.descriptorCount=bindings[i][3],.stageFlags=bindings[i][4]};
        VkDescriptorSetLayoutCreateInfo ci={.sType=VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,.bindingCount=n,.pBindings=entries};
        CHECK(vkCreateDescriptorSetLayout(device,&ci,NULL,&sets[set]));
    }
    VkPushConstantRange push={h.push_stages,h.push_offset,h.push_size};
    VkPipelineLayoutCreateInfo lci={.sType=VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,.setLayoutCount=set_count,.pSetLayouts=sets,.pushConstantRangeCount=h.push_size?1:0,.pPushConstantRanges=&push};
    VkPipelineLayout layout;CHECK(vkCreatePipelineLayout(device,&lci,NULL,&layout));
    VkShaderModuleCreateInfo sci={.sType=VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,.codeSize=h.bytes,.pCode=code};
    VkShaderModule shader;CHECK(vkCreateShaderModule(device,&sci,NULL,&shader));free(code);
    VkSpecializationMapEntry entries[64];uint32_t values[64];
    for(unsigned i=0;i<h.specs;i++){entries[i]=(VkSpecializationMapEntry){specializations[i][0],i*4,4};values[i]=specializations[i][1];}
    VkSpecializationInfo spec={h.specs,entries,h.specs*4,values};
    VkComputePipelineCreateInfo pci={.sType=VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,.layout=layout,.flags=VK_PIPELINE_CREATE_CAPTURE_STATISTICS_BIT_KHR,
        .stage={.sType=VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,.stage=VK_SHADER_STAGE_COMPUTE_BIT,.module=shader,.pName="main",.pSpecializationInfo=&spec}};
    VkPipeline pipeline;
    if(getenv("NPS64_PIPELINE_NO_OPT"))pci.flags|=VK_PIPELINE_CREATE_DISABLE_OPTIMIZATION_BIT;
#ifdef NPS64_PIPELINE_CACHE_TEST
    struct timespec before,after;clock_gettime(CLOCK_MONOTONIC,&before);
#endif
#ifdef NPS64_PIPELINE_GUARD_TEST
    PFN_vkCreateComputePipelines create_pipeline=(PFN_vkCreateComputePipelines)device_proc(device,"vkCreateComputePipelines");
    CHECK(create_pipeline(device,pipeline_cache,1,&pci,NULL,&pipeline));
#else
    CHECK(vkCreateComputePipelines(device,pipeline_cache,1,&pci,NULL,&pipeline));
#endif
#ifdef NPS64_PIPELINE_CACHE_TEST
    clock_gettime(CLOCK_MONOTONIC,&after);
    printf("CACHE compile_ms=%.3f\n",1000.0*(after.tv_sec-before.tv_sec)+(after.tv_nsec-before.tv_nsec)/1e6);
#endif
    VkPipelineInfoKHR pi={.sType=VK_STRUCTURE_TYPE_PIPELINE_INFO_KHR,.pipeline=pipeline};
    uint32_t executable_count=0;CHECK(get_properties(device,&pi,&executable_count,NULL));
    if(!executable_count || executable_count>16)return 2;
    for(uint32_t index=0;index<executable_count;index++) {
        VkPipelineExecutableInfoKHR ei={.sType=VK_STRUCTURE_TYPE_PIPELINE_EXECUTABLE_INFO_KHR,.pipeline=pipeline,.executableIndex=index};
        uint32_t n=0;CHECK(get_statistics(device,&ei,&n,NULL));
        if(n>256)return 2;
        VkPipelineExecutableStatisticKHR *stats=calloc(n,sizeof(*stats));if(!stats)return 2;
        for(uint32_t i=0;i<n;i++)stats[i].sType=VK_STRUCTURE_TYPE_PIPELINE_EXECUTABLE_STATISTIC_KHR;
        CHECK(get_statistics(device,&ei,&n,stats));
        for(uint32_t i=0;i<n;i++) {
            if(stats[i].format==VK_PIPELINE_EXECUTABLE_STATISTIC_FORMAT_UINT64_KHR)
                printf("[pipeline-stat] %s=%llu\n",stats[i].name,(unsigned long long)stats[i].value.u64);
            else if(stats[i].format==VK_PIPELINE_EXECUTABLE_STATISTIC_FORMAT_INT64_KHR)
                printf("[pipeline-stat] %s=%lld\n",stats[i].name,(long long)stats[i].value.i64);
        }
        free(stats);
    }
#ifdef NPS64_PIPELINE_CACHE_TEST
    size_t bytes=0;CHECK(vkGetPipelineCacheData(device,pipeline_cache,&bytes,NULL));
    if(bytes<=32 || bytes>NPS64_GPU_CACHE_LIMIT)return 2;
    void *blob=malloc(bytes);if(!blob)return 2;
    CHECK(vkGetPipelineCacheData(device,pipeline_cache,&bytes,blob));
    if(!nps64_gpu_cache_save(cache_dir,&identity,blob,bytes))return 2;
    free(blob);printf("CACHE saved_bytes=%zu\n",bytes);
    vkDestroyPipelineCache(device,pipeline_cache,NULL);
#endif
    vkDestroyPipeline(device,pipeline,NULL);vkDestroyShaderModule(device,shader,NULL);vkDestroyPipelineLayout(device,layout,NULL);
    for(unsigned i=0;i<set_count;i++)vkDestroyDescriptorSetLayout(device,sets[i],NULL);
    vkDestroyDevice(device,NULL);vkDestroyInstance(instance,NULL);
    printf("PASS pipeline compiled; no dispatch, fence or output test\n");return 0;
}
#ifndef __ORBIS__
int main(int argc,char **argv){return argc==2?nps64_compile_captured_pipeline(argv[1]):2;}
#endif
