/* ROM-independent PS4/host compute gate for the ParaLLEl-RDP port. */
#include <vulkan/vulkan.h>
#include "parallel_compute_probe.h"
#include "parallel_probe_spv.h"
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#ifdef __ORBIS__
#include "parallel_probe_dispatch.h"
#endif

enum { DATA_WORDS=4096, GUARD_WORDS=16, INDIRECT_WORD=DATA_WORDS+GUARD_WORDS,
       TOTAL_WORDS=INDIRECT_WORD+3 };
typedef struct {
    const char *name;
    uint32_t local[3], groups[3], mode;
    unsigned indirect, switch_pipeline;
} ProbeCase;
static const ProbeCase cases[PARALLEL_PROBE_CASES]={
    {"storage 1D / 64 lanes",{64,1,1},{4,1,1},0,0,0},
    {"storage 2D / 8x8 lanes",{8,8,1},{5,3,1},0,0,0},
    {"storage 3D / 4x4x4 lanes",{4,4,4},{2,3,2},0,0,0},
    {"LDS + atomic + barrier / 32 lanes",{32,1,1},{4,1,1},1,0,0},
    {"LDS + atomic + barrier / 64 lanes",{64,1,1},{4,1,1},1,0,0},
    {"compute read-after-write + pipeline switch",{64,1,1},{4,1,1},2,0,1},
    {"indirect dispatch + read-after-write",{8,8,1},{5,3,1},2,1,0},
};
struct ParallelComputeProbe {
    ParallelProbeReport report;
    VkInstance instance;
    VkPhysicalDevice physical;
    VkDevice device;
    VkQueue queue;
    VkBuffer buffer;
    VkDeviceMemory memory;
    uint32_t *mapped;
    VkDescriptorSetLayout set_layout;
    VkPipelineLayout layout;
    VkDescriptorPool descriptors;
    VkDescriptorSet set;
    VkShaderModule shader;
    VkCommandPool commands;
    VkCommandBuffer cmd;
    VkFence fence;
    VkPipeline pipelines[PARALLEL_PROBE_CASES], storage_pipeline;
    unsigned failed;
};
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

const char *parallel_probe_case_name(unsigned index) {
    return index<PARALLEL_PROBE_CASES?cases[index].name:"invalid case";
}
static int create_pipeline(ParallelComputeProbe *p,const ProbeCase *c,uint32_t mode,VkPipeline *pipeline) {
    VkSpecializationMapEntry entries[]={{0,0,4},{1,4,4},{2,8,4},{3,12,4}};
    uint32_t values[]={c->local[0],mode,c->local[1],c->local[2]};
    VkSpecializationInfo spec={4,entries,sizeof(values),values};
    VkComputePipelineCreateInfo ci={.sType=VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
        .stage={.sType=VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage=VK_SHADER_STAGE_COMPUTE_BIT,.module=p->shader,.pName="main",.pSpecializationInfo=&spec},
        .layout=p->layout};
    TRY(vkCreateComputePipelines(p->device,VK_NULL_HANDLE,1,&ci,NULL,pipeline));return 0;
}
int parallel_probe_open(ParallelComputeProbe **output,ParallelProbeReport callback) {
    if(!output||*output||!callback)return 1;
    ParallelComputeProbe *p=calloc(1,sizeof(*p));if(!p)return 1;
    p->report=callback;*output=p;
#ifdef __ORBIS__
    if(!parallel_load_global())return failure(p,"global Vulkan dispatch",VK_ERROR_INITIALIZATION_FAILED);
#endif
    VkApplicationInfo app={.sType=VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName="NPS64 00.52 ParaLLEl compute gate",.apiVersion=VK_API_VERSION_1_1};
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
    report(p,"device=%s api=%u; compute gate, not game emulation",props.deviceName,props.apiVersion);
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
    VkDeviceCreateInfo dci={.sType=VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,.queueCreateInfoCount=1,.pQueueCreateInfos=&qci};
    TRY(vkCreateDevice(p->physical,&dci,NULL,&p->device));
#ifdef __ORBIS__
    if(!parallel_load_device(p->device))return failure(p,"device Vulkan dispatch",VK_ERROR_INITIALIZATION_FAILED);
#endif
    vkGetDeviceQueue(p->device,family,0,&p->queue);
    VkBufferCreateInfo bci={.sType=VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size=TOTAL_WORDS*sizeof(uint32_t),.usage=VK_BUFFER_USAGE_STORAGE_BUFFER_BIT|VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT,
        .sharingMode=VK_SHARING_MODE_EXCLUSIVE};
    TRY(vkCreateBuffer(p->device,&bci,NULL,&p->buffer));
    VkMemoryRequirements req;vkGetBufferMemoryRequirements(p->device,p->buffer,&req);
    VkPhysicalDeviceMemoryProperties mp;vkGetPhysicalDeviceMemoryProperties(p->physical,&mp);
    uint32_t mt=mp.memoryTypeCount;
    const VkMemoryPropertyFlags required=VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    for(uint32_t i=0;i<mp.memoryTypeCount;i++)if((req.memoryTypeBits&(1u<<i))&&(mp.memoryTypes[i].propertyFlags&required)==required){mt=i;break;}
    if(mt==mp.memoryTypeCount)return failure(p,"no coherent memory",VK_ERROR_FEATURE_NOT_PRESENT);
    VkMemoryAllocateInfo mai={.sType=VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,.allocationSize=req.size,.memoryTypeIndex=mt};
    TRY(vkAllocateMemory(p->device,&mai,NULL,&p->memory));
    TRY(vkBindBufferMemory(p->device,p->buffer,p->memory,0));
    TRY(vkMapMemory(p->device,p->memory,0,VK_WHOLE_SIZE,0,(void**)&p->mapped));
    VkDescriptorSetLayoutBinding binding={.binding=0,.descriptorType=VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,.descriptorCount=1,.stageFlags=VK_SHADER_STAGE_COMPUTE_BIT};
    VkDescriptorSetLayoutCreateInfo sci={.sType=VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,.bindingCount=1,.pBindings=&binding};
    TRY(vkCreateDescriptorSetLayout(p->device,&sci,NULL,&p->set_layout));
    VkPushConstantRange push={VK_SHADER_STAGE_COMPUTE_BIT,0,8};
    VkPipelineLayoutCreateInfo lci={.sType=VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,.setLayoutCount=1,.pSetLayouts=&p->set_layout,.pushConstantRangeCount=1,.pPushConstantRanges=&push};
    TRY(vkCreatePipelineLayout(p->device,&lci,NULL,&p->layout));
    VkDescriptorPoolSize size={VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,1};
    VkDescriptorPoolCreateInfo dpci={.sType=VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,.maxSets=1,.poolSizeCount=1,.pPoolSizes=&size};
    TRY(vkCreateDescriptorPool(p->device,&dpci,NULL,&p->descriptors));
    VkDescriptorSetAllocateInfo dsai={.sType=VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,.descriptorPool=p->descriptors,.descriptorSetCount=1,.pSetLayouts=&p->set_layout};
    TRY(vkAllocateDescriptorSets(p->device,&dsai,&p->set));
    VkDescriptorBufferInfo dbi={p->buffer,0,INDIRECT_WORD*sizeof(uint32_t)};
    VkWriteDescriptorSet write={.sType=VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,.dstSet=p->set,.descriptorCount=1,.descriptorType=VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,.pBufferInfo=&dbi};
    vkUpdateDescriptorSets(p->device,1,&write,0,NULL);
    VkShaderModuleCreateInfo smci={.sType=VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,.codeSize=sizeof(parallel_probe_spv),.pCode=parallel_probe_spv};
    TRY(vkCreateShaderModule(p->device,&smci,NULL,&p->shader));
    VkCommandPoolCreateInfo cpci={.sType=VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,.flags=VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,.queueFamilyIndex=family};
    TRY(vkCreateCommandPool(p->device,&cpci,NULL,&p->commands));
    VkCommandBufferAllocateInfo cbai={.sType=VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,.commandPool=p->commands,.level=VK_COMMAND_BUFFER_LEVEL_PRIMARY,.commandBufferCount=1};
    TRY(vkAllocateCommandBuffers(p->device,&cbai,&p->cmd));
    VkFenceCreateInfo fci={.sType=VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};TRY(vkCreateFence(p->device,&fci,NULL,&p->fence));
    report(p,"READY seven isolated cases; every dispatch checked after fence");return 0;
}
int parallel_probe_run(ParallelComputeProbe *p,unsigned index,unsigned repeats) {
    if(!p||p->failed||index>=PARALLEL_PROBE_CASES||!repeats||repeats>PARALLEL_PROBE_REPEATS)return 1;
    const ProbeCase *c=&cases[index];
    report(p,"BEGIN case=%u %s local=%ux%ux%u groups=%ux%ux%u repeats=%u",index,c->name,
        c->local[0],c->local[1],c->local[2],c->groups[0],c->groups[1],c->groups[2],repeats);
    if(!p->pipelines[index]&&create_pipeline(p,c,c->mode,&p->pipelines[index]))return 1;
    if(c->switch_pipeline&&!p->storage_pipeline&&create_pipeline(p,c,0,&p->storage_pipeline))return 1;
    uint32_t length=1;for(unsigned i=0;i<3;i++)length*=c->local[i]*c->groups[i];
    if(length>DATA_WORDS)return failure(p,"case capacity",VK_ERROR_INITIALIZATION_FAILED);
    for(unsigned r=0;r<repeats;r++) {
        uint32_t salt=0x01010101u*(r+1);
        for(unsigned i=0;i<TOTAL_WORDS;i++)p->mapped[i]=0xcdcdcdcdu;
        for(unsigned i=0;i<3;i++)p->mapped[INDIRECT_WORD+i]=c->groups[i];
        TRY(vkResetCommandBuffer(p->cmd,0));
        VkCommandBufferBeginInfo begin={.sType=VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};TRY(vkBeginCommandBuffer(p->cmd,&begin));
        VkMemoryBarrier upload={.sType=VK_STRUCTURE_TYPE_MEMORY_BARRIER,.srcAccessMask=VK_ACCESS_HOST_WRITE_BIT,
            .dstAccessMask=VK_ACCESS_SHADER_READ_BIT|VK_ACCESS_SHADER_WRITE_BIT|VK_ACCESS_INDIRECT_COMMAND_READ_BIT};
        vkCmdPipelineBarrier(p->cmd,VK_PIPELINE_STAGE_HOST_BIT,VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT|VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT,0,1,&upload,0,NULL,0,NULL);
        vkCmdBindDescriptorSets(p->cmd,VK_PIPELINE_BIND_POINT_COMPUTE,p->layout,0,1,&p->set,0,NULL);
        for(uint32_t phase=0;phase<2;phase++) {
            VkPipeline pipeline=c->switch_pipeline&&phase==0?p->storage_pipeline:p->pipelines[index];
            vkCmdBindPipeline(p->cmd,VK_PIPELINE_BIND_POINT_COMPUTE,pipeline);
            uint32_t push[]={phase,salt};vkCmdPushConstants(p->cmd,p->layout,VK_SHADER_STAGE_COMPUTE_BIT,0,sizeof(push),push);
            if(c->indirect)vkCmdDispatchIndirect(p->cmd,p->buffer,INDIRECT_WORD*sizeof(uint32_t));
            else vkCmdDispatch(p->cmd,c->groups[0],c->groups[1],c->groups[2]);
            VkMemoryBarrier barrier={.sType=VK_STRUCTURE_TYPE_MEMORY_BARRIER,.srcAccessMask=VK_ACCESS_SHADER_WRITE_BIT,
                .dstAccessMask=phase?VK_ACCESS_HOST_READ_BIT:VK_ACCESS_SHADER_READ_BIT|VK_ACCESS_SHADER_WRITE_BIT};
            vkCmdPipelineBarrier(p->cmd,VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                phase?VK_PIPELINE_STAGE_HOST_BIT:VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,0,1,&barrier,0,NULL,0,NULL);
        }
        TRY(vkEndCommandBuffer(p->cmd));
        report(p,"SUBMIT case=%u repeat=%u; two dispatches; waiting fence",index,r);
        VkSubmitInfo submit={.sType=VK_STRUCTURE_TYPE_SUBMIT_INFO,.commandBufferCount=1,.pCommandBuffers=&p->cmd};
        TRY(vkQueueSubmit(p->queue,1,&submit,p->fence));
        TRY(vkWaitForFences(p->device,1,&p->fence,VK_TRUE,5000000000ull));
        for(unsigned i=0;i<TOTAL_WORDS;i++) {
            uint32_t expected=0xcdcdcdcdu;
            if(i<length){expected=c->mode==1?32u^salt:i^0x64a5u^salt;if(c->mode==2)expected^=0x5a5a5a5au;}
            if(i>=INDIRECT_WORD)expected=c->groups[i-INDIRECT_WORD];
            if(p->mapped[i]!=expected){p->failed=1;report(p,"FAIL case=%u repeat=%u word=%u actual=%08x expected=%08x",index,r,i,p->mapped[i],expected);return 1;}
        }
        TRY(vkResetFences(p->device,1,&p->fence));
        report(p,"COMPLETE case=%u repeat=%u fence=success words=%u guard=exact",index,r,length);
    }
    report(p,"PASS case=%u %s: %u completed submissions, exact words and guard",index,c->name,repeats);return 0;
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
        if(p->storage_pipeline)vkDestroyPipeline(p->device,p->storage_pipeline,NULL);
        if(p->fence)vkDestroyFence(p->device,p->fence,NULL);
        if(p->commands)vkDestroyCommandPool(p->device,p->commands,NULL);
        if(p->shader)vkDestroyShaderModule(p->device,p->shader,NULL);
        if(p->descriptors)vkDestroyDescriptorPool(p->device,p->descriptors,NULL);
        if(p->layout)vkDestroyPipelineLayout(p->device,p->layout,NULL);
        if(p->set_layout)vkDestroyDescriptorSetLayout(p->device,p->set_layout,NULL);
        if(p->mapped)vkUnmapMemory(p->device,p->memory);
        if(p->buffer)vkDestroyBuffer(p->device,p->buffer,NULL);
        if(p->memory)vkFreeMemory(p->device,p->memory,NULL);
        vkDestroyDevice(p->device,NULL);
    }
    if(p->instance)vkDestroyInstance(p->instance,NULL);free(p);
#endif
}
