/* Original ParaLLEl tile stage with externally supplied PS4 capture inputs.
 * Cases 0/1 preserve the original shader at 32/64 lanes. Cases 2/3 isolate
 * shared-memory and atomic operations; case 3 is ONLY valid for this fixture.
 * Host reference compares every mask word, both inputs and allocation guards.
 * Captured ROM-derived data is generated under ignored build/, never in Git. */
#include <vulkan/vulkan.h>
#include "parallel_compute_probe.h"
#include "parallel_tile_data.h"
#include "parallel_tile_direct_data.h"
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#ifdef __ORBIS__
#include "parallel_probe_dispatch.h"
#endif

enum { SLOT_BYTES=1024*1024, TOTAL_BYTES=5*SLOT_BYTES };
static const unsigned sizes[5]={8192,4096,1024,524288,65536};
struct ParallelComputeProbe {
 ParallelProbeReport report;
 VkInstance instance; VkPhysicalDevice physical; VkDevice device; VkQueue queue;
 VkBuffer buffers[5]; VkDeviceMemory memories[5]; uint32_t *mapped[5];
 VkDescriptorSetLayout set_layout; VkPipelineLayout layout;
 VkDescriptorPool descriptors; VkDescriptorSet set;
 VkCommandPool commands; VkCommandBuffer cmd; VkFence fence;
 VkShaderModule shaders[PARALLEL_PROBE_CASES]; VkPipeline pipelines[PARALLEL_PROBE_CASES]; unsigned failed;
};
const char *parallel_probe_case_name(unsigned index) {
 static const char *names[]={"RDP original / 32 lanes","RDP original / 64 lanes (candidate)",
  "Direct tiles / no shared LDS / atomics","Direct tiles / stores / one-group fixture only",
  "Direct tiles / 0..256 primitives / concurrent groups",
  "Direct tiles / same stress / GPU input uploads"};
 return index<PARALLEL_PROBE_CASES?names[index]:"invalid";
}
static void report(ParallelComputeProbe *p,const char *format,...) {
    char line[512];va_list args;va_start(args,format);
    vsnprintf(line,sizeof(line),format,args);va_end(args);
    if(p->report)p->report(line);
}
static int failure(ParallelComputeProbe *p,const char *operation,VkResult result) {
    p->failed=1;report(p,"FAIL %s result=%d; no further submissions",operation,result);
    return result==VK_TIMEOUT?2:1;
}
#define TRY(call) do {VkResult r=(call);if(r!=VK_SUCCESS)return failure(p,#call,r);}while(0)

int parallel_probe_open(ParallelComputeProbe **output,ParallelProbeReport callback) {
    if(!output||*output||!callback)return 1;
    ParallelComputeProbe *p=calloc(1,sizeof(*p));if(!p)return 1;
    p->report=callback;*output=p;
#ifdef __ORBIS__
    if(!parallel_load_global())return failure(p,"global Vulkan dispatch",VK_ERROR_INITIALIZATION_FAILED);
#endif
    VkApplicationInfo app={.sType=VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName="NPS64 00.55 original tile capture",.apiVersion=VK_API_VERSION_1_2};
    VkInstanceCreateInfo ici={.sType=VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,.pApplicationInfo=&app};
    TRY(vkCreateInstance(&ici,NULL,&p->instance));
#ifdef __ORBIS__
    if(!parallel_load_instance(p->instance))return failure(p,"instance Vulkan dispatch",VK_ERROR_INITIALIZATION_FAILED);
#endif
    uint32_t count=0;TRY(vkEnumeratePhysicalDevices(p->instance,&count,NULL));
    if(!count)return failure(p,"no physical device",VK_ERROR_INITIALIZATION_FAILED);
    VkPhysicalDevice *physical=calloc(count,sizeof(*physical));if(!physical)return failure(p,"device list",VK_ERROR_OUT_OF_HOST_MEMORY);
    VkResult result=vkEnumeratePhysicalDevices(p->instance,&count,physical);
    if(result==VK_SUCCESS)p->physical=physical[0];free(physical);
    if(result!=VK_SUCCESS)return failure(p,"enumerate physical devices",result);
    VkPhysicalDeviceProperties props;vkGetPhysicalDeviceProperties(p->physical,&props);
    report(p,"device=%s api=%u; isolated RDP stage, not game emulation",props.deviceName,props.apiVersion);
    vkGetPhysicalDeviceQueueFamilyProperties(p->physical,&count,NULL);
    VkQueueFamilyProperties *families=calloc(count,sizeof(*families));if(!families)return failure(p,"queue list",VK_ERROR_OUT_OF_HOST_MEMORY);
    vkGetPhysicalDeviceQueueFamilyProperties(p->physical,&count,families);
    uint32_t family=count;
    for(uint32_t i=0;i<count;i++)if(families[i].queueCount&&
       (families[i].queueFlags&(VK_QUEUE_GRAPHICS_BIT|VK_QUEUE_COMPUTE_BIT))==(VK_QUEUE_GRAPHICS_BIT|VK_QUEUE_COMPUTE_BIT)){family=i;break;}
    if(family==count)for(uint32_t i=0;i<count;i++)if(families[i].queueCount&&(families[i].queueFlags&VK_QUEUE_COMPUTE_BIT)){family=i;break;}
    free(families);if(family==count)return failure(p,"no compute queue",VK_ERROR_INITIALIZATION_FAILED);
    report(p,"queue_family=%u; host-visible coherent storage",family);
    float priority=1;
    VkDeviceQueueCreateInfo qci={.sType=VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,.queueFamilyIndex=family,.queueCount=1,.pQueuePriorities=&priority};
    /* The upstream SPIR-V declares storage capabilities even with SMALL_TYPES=0. */
    VkPhysicalDeviceVulkan11Features f11={.sType=VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES};
    VkPhysicalDeviceVulkan12Features f12={.sType=VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES,.pNext=&f11};
    VkPhysicalDeviceFeatures2 features={.sType=VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,.pNext=&f12};
    vkGetPhysicalDeviceFeatures2(p->physical,&features);
    if(!f11.storageBuffer16BitAccess||!f12.storageBuffer8BitAccess)return failure(p,"required upstream storage capabilities",VK_ERROR_FEATURE_NOT_PRESENT);
    f11=(VkPhysicalDeviceVulkan11Features){.sType=VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES,.storageBuffer16BitAccess=VK_TRUE};
    f12=(VkPhysicalDeviceVulkan12Features){.sType=VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES,.pNext=&f11,.storageBuffer8BitAccess=VK_TRUE};
    VkDeviceCreateInfo dci={.sType=VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,.pNext=&f12,.queueCreateInfoCount=1,.pQueueCreateInfos=&qci};
    TRY(vkCreateDevice(p->physical,&dci,NULL,&p->device));
#ifdef __ORBIS__
    if(!parallel_load_device(p->device))return failure(p,"device Vulkan dispatch",VK_ERROR_INITIALIZATION_FAILED);
#endif
    vkGetDeviceQueue(p->device,family,0,&p->queue);
    VkPhysicalDeviceMemoryProperties mp;vkGetPhysicalDeviceMemoryProperties(p->physical,&mp);
    const VkMemoryPropertyFlags required=VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    /* Separate allocations reproduce the distinct input/output BOs of RDP. */
    for(unsigned binding=0;binding<5;binding++){
        VkBufferCreateInfo bci={.sType=VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
            .size=SLOT_BYTES,.usage=VK_BUFFER_USAGE_STORAGE_BUFFER_BIT|VK_BUFFER_USAGE_TRANSFER_DST_BIT,.sharingMode=VK_SHARING_MODE_EXCLUSIVE};
        TRY(vkCreateBuffer(p->device,&bci,NULL,&p->buffers[binding]));
        VkMemoryRequirements req;vkGetBufferMemoryRequirements(p->device,p->buffers[binding],&req);
        unsigned mt=mp.memoryTypeCount;
        for(unsigned i=0;i<mp.memoryTypeCount;i++)if((req.memoryTypeBits&(1u<<i))&&(mp.memoryTypes[i].propertyFlags&required)==required){mt=i;break;}
        if(mt==mp.memoryTypeCount)return failure(p,"no coherent memory",VK_ERROR_FEATURE_NOT_PRESENT);
        VkMemoryAllocateInfo mai={.sType=VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,.allocationSize=req.size,.memoryTypeIndex=mt};
        TRY(vkAllocateMemory(p->device,&mai,NULL,&p->memories[binding]));
        TRY(vkBindBufferMemory(p->device,p->buffers[binding],p->memories[binding],0));
        TRY(vkMapMemory(p->device,p->memories[binding],0,VK_WHOLE_SIZE,0,(void**)&p->mapped[binding]));
    }
    report(p,"CREATE descriptor layout begin");
    VkDescriptorSetLayoutBinding bindings[5];
    for(unsigned i=0;i<5;i++)bindings[i]=(VkDescriptorSetLayoutBinding){.binding=i,.descriptorType=VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,.descriptorCount=1,.stageFlags=VK_SHADER_STAGE_COMPUTE_BIT};
    VkDescriptorSetLayoutCreateInfo slci={.sType=VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,.bindingCount=5,.pBindings=bindings};
    TRY(vkCreateDescriptorSetLayout(p->device,&slci,NULL,&p->set_layout));
    VkPushConstantRange push={VK_SHADER_STAGE_COMPUTE_BIT,0,12};
    VkPipelineLayoutCreateInfo plci={.sType=VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,.setLayoutCount=1,.pSetLayouts=&p->set_layout,.pushConstantRangeCount=1,.pPushConstantRanges=&push};
    TRY(vkCreatePipelineLayout(p->device,&plci,NULL,&p->layout));
    VkDescriptorPoolSize size={VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,5};
    VkDescriptorPoolCreateInfo dpci={.sType=VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,.maxSets=1,.poolSizeCount=1,.pPoolSizes=&size};
    TRY(vkCreateDescriptorPool(p->device,&dpci,NULL,&p->descriptors));
    VkDescriptorSetAllocateInfo dsai={.sType=VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,.descriptorPool=p->descriptors,.descriptorSetCount=1,.pSetLayouts=&p->set_layout};
    TRY(vkAllocateDescriptorSets(p->device,&dsai,&p->set));
    for(unsigned i=0;i<5;i++){
        VkDescriptorBufferInfo dbi={p->buffers[i],0,sizes[i]};
        VkWriteDescriptorSet write={.sType=VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,.dstSet=p->set,.dstBinding=i,.descriptorCount=1,.descriptorType=VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,.pBufferInfo=&dbi};
        vkUpdateDescriptorSets(p->device,1,&write,0,NULL);
    }
    report(p,"CREATE command pool and fence begin");
    VkCommandPoolCreateInfo cpci={.sType=VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,.flags=VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,.queueFamilyIndex=family};
    TRY(vkCreateCommandPool(p->device,&cpci,NULL,&p->commands));
    VkCommandBufferAllocateInfo cbai={.sType=VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,.commandPool=p->commands,.level=VK_COMMAND_BUFFER_LEVEL_PRIMARY,.commandBufferCount=1};
    TRY(vkAllocateCommandBuffers(p->device,&cbai,&p->cmd));
    VkFenceCreateInfo fci={.sType=VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};TRY(vkCreateFence(p->device,&fci,NULL,&p->fence));
    report(p,"READY isolated first tile stage: 320x239, one primitive; original 32 vs candidate 64");return 0;
}
static int prepare_pipeline(ParallelComputeProbe *p,unsigned index){
    const uint32_t *codes[]={tile_spv32,tile_spv64,tile_spv_direct_atomic,tile_spv_direct_store,tile_spv_direct_atomic,tile_spv_direct_atomic};
    const size_t bytes[]={sizeof(tile_spv32),sizeof(tile_spv64),sizeof(tile_spv_direct_atomic),sizeof(tile_spv_direct_store),sizeof(tile_spv_direct_atomic),sizeof(tile_spv_direct_atomic)};
    unsigned i=index;
    {
        VkShaderModuleCreateInfo smci={.sType=VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,.codeSize=bytes[i],.pCode=codes[i]};
        report(p,"CREATE shader case=%u bytes=%zu",i,bytes[i]);
        TRY(vkCreateShaderModule(p->device,&smci,NULL,&p->shaders[i]));
        VkSpecializationMapEntry entries[7];for(unsigned j=0;j<7;j++)entries[j]=(VkSpecializationMapEntry){j,4*j,4};
        uint32_t constants[]={i==1?64:32,8,8,256,1024,32768,1};
        VkSpecializationInfo spec={7,entries,sizeof(constants),constants};
        VkComputePipelineCreateInfo pci={.sType=VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
          .stage={.sType=VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,.stage=VK_SHADER_STAGE_COMPUTE_BIT,.module=p->shaders[i],.pName="main",.pSpecializationInfo=&spec},.layout=p->layout};
        report(p,"COMPILE pipeline case=%u local=%u begin",i,i==1?64:32);
        TRY(vkCreateComputePipelines(p->device,VK_NULL_HANDLE,1,&pci,NULL,&p->pipelines[i]));
        report(p,"COMPILE pipeline case=%u complete",i);
    }
    return 0;
}
/* Independent CPU oracle for a captured full-screen rectangle. Tile-aligned
 * quadrant scissors provide distinct masks without copying shader logic. */
static void stress_fixture(uint32_t *expected,unsigned count,unsigned quadrants) {
    uint32_t *scissors=(uint32_t*)((char*)expected+SLOT_BYTES);
    uint32_t *mask=(uint32_t*)((char*)expected+3*SLOT_BYTES);
    uint32_t *coarse=(uint32_t*)((char*)expected+4*SLOT_BYTES);
    memset(mask,0,sizes[3]);memset(coarse,0,sizes[4]);
    for(unsigned primitive=0;primitive<256;primitive++){
        memcpy(expected+primitive*8,tile_triangle,32);
        uint32_t *scissor=scissors+primitive*4;
        if(!quadrants)memcpy(scissor,tile_scissor,16);
        else{
            unsigned right=primitive&1,bottom=(primitive>>1)&1;
            scissor[0]=right?640:0;scissor[1]=bottom?480:0;
            scissor[2]=right?1280:640;scissor[3]=bottom?960:480;
        }
    }
    for(unsigned y=0;y<30;y++)for(unsigned x=0;x<40;x++){
        unsigned tile=y*128+x;
        for(unsigned primitive=0;primitive<count;primitive++){
            /* The captured FILL/COPY rectangle has an inclusive X scissor:
             * x=160 belongs to both horizontal halves (tile 20). */
            if(quadrants){
                unsigned right=primitive&1,bottom=(primitive>>1)&1;
                if((right?x<20:x>20)||(bottom?y<15:y>=15))continue;
            }
            unsigned group=primitive/32;
            mask[tile*8+group]|=1u<<(primitive&31);
            coarse[tile]|=1u<<group;
        }
    }
}
int parallel_probe_run(ParallelComputeProbe *p,unsigned index,unsigned repeats) {
    if(!p||p->failed||index>=PARALLEL_PROBE_CASES||!repeats||repeats>PARALLEL_PROBE_REPEATS)return 1;
    if(!p->pipelines[index]&&prepare_pipeline(p,index))return 1;
#ifndef __ORBIS__
    /* drm-shim can compile AMD ISA but cannot execute or validate pixels. */
    if(getenv("NPS64_TILE_COMPILE_ONLY")){
        report(p,"COMPILE ONLY case=%u complete; no GPU dispatch or output validation",index);return 0;
    }
#endif
    uint32_t *expected=malloc(TOTAL_BYTES);if(!expected)return failure(p,"reference allocation",VK_ERROR_OUT_OF_HOST_MEMORY);
    memset(expected,0xcd,TOTAL_BYTES);
    for(unsigned i=0;i<5;i++)memset((char*)expected+i*SLOT_BYTES,0,sizes[i]);
    memcpy(expected,tile_triangle,sizeof(tile_triangle));
    memcpy((char*)expected+SLOT_BYTES,tile_scissor,sizeof(tile_scissor));
    memcpy((char*)expected+3*SLOT_BYTES,tile_expected_mask,sizeof(tile_expected_mask));
    memcpy((char*)expected+4*SLOT_BYTES,tile_expected_coarse,sizeof(tile_expected_coarse));
    report(p,"BEGIN case=%u %s groups=1x5x%u repeats=%u",index,parallel_probe_case_name(index),index==1?4:8,repeats);
    for(unsigned r=0;r<repeats;r++){
        static const unsigned counts[]={0,1,31,32,33,63,65,255,256};
        unsigned primitives=index>=4?counts[r%9]:1;
        unsigned groups=(primitives+31)/32;
        if(index>=4){
            unsigned quadrants=(r/9)&1;
            stress_fixture(expected,primitives,quadrants);
            report(p,"STRESS primitives=%u groups=%u quadrant_scissors=%u",primitives,groups,quadrants);
        }
        for(unsigned i=0;i<5;i++)memcpy(p->mapped[i],(char*)expected+i*SLOT_BYTES,SLOT_BYTES);
        memset(p->mapped[3],0,sizes[3]);memset(p->mapped[4],0,sizes[4]);
        TRY(vkResetCommandBuffer(p->cmd,0));
        VkCommandBufferBeginInfo begin={.sType=VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};TRY(vkBeginCommandBuffer(p->cmd,&begin));
        VkMemoryBarrier upload={.sType=VK_STRUCTURE_TYPE_MEMORY_BARRIER,.srcAccessMask=VK_ACCESS_HOST_WRITE_BIT,.dstAccessMask=VK_ACCESS_SHADER_READ_BIT|VK_ACCESS_SHADER_WRITE_BIT};
        vkCmdPipelineBarrier(p->cmd,VK_PIPELINE_STAGE_HOST_BIT,VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,0,1,&upload,0,NULL,0,NULL);
        if(index==5){
            VkMemoryBarrier before={.sType=VK_STRUCTURE_TYPE_MEMORY_BARRIER,.srcAccessMask=VK_ACCESS_HOST_WRITE_BIT,.dstAccessMask=VK_ACCESS_TRANSFER_WRITE_BIT};
            vkCmdPipelineBarrier(p->cmd,VK_PIPELINE_STAGE_HOST_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT,0,1,&before,0,NULL,0,NULL);
            vkCmdUpdateBuffer(p->cmd,p->buffers[0],0,sizes[0],expected);
            vkCmdUpdateBuffer(p->cmd,p->buffers[1],0,sizes[1],(char*)expected+SLOT_BYTES);
            VkMemoryBarrier after={.sType=VK_STRUCTURE_TYPE_MEMORY_BARRIER,.srcAccessMask=VK_ACCESS_TRANSFER_WRITE_BIT,.dstAccessMask=VK_ACCESS_SHADER_READ_BIT|VK_ACCESS_HOST_READ_BIT};
            vkCmdPipelineBarrier(p->cmd,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT|VK_PIPELINE_STAGE_HOST_BIT,0,1,&after,0,NULL,0,NULL);
        }
        vkCmdBindPipeline(p->cmd,VK_PIPELINE_BIND_POINT_COMPUTE,p->pipelines[index]);
        vkCmdBindDescriptorSets(p->cmd,VK_PIPELINE_BIND_POINT_COMPUTE,p->layout,0,1,&p->set,0,NULL);
        const uint32_t registers[]={320,239,primitives};vkCmdPushConstants(p->cmd,p->layout,VK_SHADER_STAGE_COMPUTE_BIT,0,sizeof(registers),registers);
        vkCmdDispatch(p->cmd,groups,5,index==1?4:8);
        VkMemoryBarrier download={.sType=VK_STRUCTURE_TYPE_MEMORY_BARRIER,.srcAccessMask=VK_ACCESS_SHADER_WRITE_BIT,.dstAccessMask=VK_ACCESS_HOST_READ_BIT};
        vkCmdPipelineBarrier(p->cmd,VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,VK_PIPELINE_STAGE_HOST_BIT,0,1,&download,0,NULL,0,NULL);
        TRY(vkEndCommandBuffer(p->cmd));
        VkSubmitInfo submit={.sType=VK_STRUCTURE_TYPE_SUBMIT_INFO,.commandBufferCount=1,.pCommandBuffers=&p->cmd};
        report(p,"SUBMIT case=%u repeat=%u original tile shader; next operation waits for fence",index,r);
        TRY(vkQueueSubmit(p->queue,1,&submit,p->fence));TRY(vkWaitForFences(p->device,1,&p->fence,VK_TRUE,5000000000ull));
#ifndef __ORBIS__
        const char *record=getenv("NPS64_TILE_RECORD");
        if(record){
            char path[1024];snprintf(path,sizeof(path),"%s-mask.bin",record);FILE *f=fopen(path,"wb");if(!f){free(expected);return 1;}
            size_t n=fwrite(p->mapped[3],1,sizes[3],f);int closed=fclose(f);if(n!=sizes[3]||closed){free(expected);return 1;}
            snprintf(path,sizeof(path),"%s-coarse.bin",record);f=fopen(path,"wb");if(!f){free(expected);return 1;}
            n=fwrite(p->mapped[4],1,sizes[4],f);closed=fclose(f);if(n!=sizes[4]||closed){free(expected);return 1;}
            memcpy((char*)expected+3*SLOT_BYTES,p->mapped[3],sizes[3]);memcpy((char*)expected+4*SLOT_BYTES,p->mapped[4],sizes[4]);
        }
#endif
        for(unsigned i=0;i<TOTAL_BYTES/4;i++)if(p->mapped[i/(SLOT_BYTES/4)][i%(SLOT_BYTES/4)]!=expected[i]){
            p->failed=1;report(p,"FAIL case=%u repeat=%u word=%u actual=%08x expected=%08x",index,r,i,p->mapped[i/(SLOT_BYTES/4)][i%(SLOT_BYTES/4)],expected[i]);
#ifdef __ORBIS__
            for(unsigned buffer=0;buffer<5;buffer++){
                char path[160];snprintf(path,sizeof(path),"/data/NPS64-Parallel/tile-fail-case%u-buffer%u.bin",index,buffer);
                FILE *file=fopen(path,"wb");
                if(file){size_t written=fwrite(p->mapped[buffer],1,SLOT_BYTES,file);int closed=fclose(file);
                    report(p,"CAPTURE buffer=%u bytes=%zu close=%d path=%s",buffer,written,closed,path);}
                else report(p,"CAPTURE failed buffer=%u",buffer);
            }
#endif
            free(expected);return 1;
        }
        TRY(vkResetFences(p->device,1,&p->fence));report(p,"COMPLETE case=%u repeat=%u fence=success all 5 MiB masks, inputs and guards exact",index,r);
    }
    free(expected);report(p,"PASS case=%u %s: %u submissions",index,parallel_probe_case_name(index),repeats);return 0;
}
void parallel_probe_close(ParallelComputeProbe *p) {
    if(!p)return;
#ifdef __ORBIS__
    /* Device destruction crashed the earlier graphics probe. Keep ownership
     * through the result screen; process exit reclaims it. No reset on hang. */
    report(p,"PS4 device retained through result screen; no GPU teardown");
#else
    if(p->failed)return; /* A lost/hung device must not enter blocking cleanup. */
    if(p->device){
        for(unsigned i=0;i<PARALLEL_PROBE_CASES;i++)if(p->pipelines[i])vkDestroyPipeline(p->device,p->pipelines[i],NULL);
        if(p->fence)vkDestroyFence(p->device,p->fence,NULL);
        if(p->commands)vkDestroyCommandPool(p->device,p->commands,NULL);
        for(unsigned i=0;i<PARALLEL_PROBE_CASES;i++)if(p->shaders[i])vkDestroyShaderModule(p->device,p->shaders[i],NULL);
        if(p->descriptors)vkDestroyDescriptorPool(p->device,p->descriptors,NULL);
        if(p->layout)vkDestroyPipelineLayout(p->device,p->layout,NULL);
        if(p->set_layout)vkDestroyDescriptorSetLayout(p->device,p->set_layout,NULL);
        for(unsigned i=0;i<5;i++){
            if(p->mapped[i])vkUnmapMemory(p->device,p->memories[i]);
            if(p->buffers[i])vkDestroyBuffer(p->device,p->buffers[i],NULL);
            if(p->memories[i])vkFreeMemory(p->device,p->memories[i],NULL);
        }
        vkDestroyDevice(p->device,NULL);
    }
    if(p->instance)vkDestroyInstance(p->instance,NULL);free(p);
#endif
}
