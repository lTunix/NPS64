/* First hardware gate for an HLE renderer: graphics pipeline, no N64 core.
 * SPDX-License-Identifier: GPL-2.0-or-later */
#include <vulkan/vulkan.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stddef.h>
#include "hle_display_list.h"
#include "hle_game_material.h"
#include "gpu_shaders/hle_probe_spv.h"
#ifdef __ORBIS__
#include "log.h"
#include "gpu_shaders/hle_probe_dispatch.h"
#define REPORT nps64_log
#else
#define REPORT printf
#endif
#define CHECK(call) do {VkResult result=(call);if(result!=VK_SUCCESS){REPORT("[hle-probe] %s failed=%d\n",#call,result);return 1;}}while(0)
typedef struct { float color[4]; float depth; uint32_t textured,wrap,padding; float rect[4],prim[4],env[4];uint32_t mux[4];float sampling[4]; } ProbeDraw;
_Static_assert(sizeof(ProbeDraw)==112,"GLSL push constant layout must remain 112 bytes");
static uint32_t memory_type(VkPhysicalDevice physical,uint32_t bits,VkMemoryPropertyFlags required) {
    VkPhysicalDeviceMemoryProperties p;vkGetPhysicalDeviceMemoryProperties(physical,&p);
    for(uint32_t i=0;i<p.memoryTypeCount;i++)if((bits&(1u<<i))&&(p.memoryTypes[i].propertyFlags&required)==required)return i;
    return UINT32_MAX;
}
static int image_create(VkDevice device,VkPhysicalDevice physical,VkFormat format,VkImageUsageFlags usage,
                        VkImageAspectFlags aspect,unsigned size,VkImage *image,VkDeviceMemory *memory,VkImageView *view) {
    VkImageCreateInfo ci={.sType=VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,.imageType=VK_IMAGE_TYPE_2D,
        .format=format,.extent={size,size,1},.mipLevels=1,.arrayLayers=1,.samples=VK_SAMPLE_COUNT_1_BIT,
        .tiling=VK_IMAGE_TILING_OPTIMAL,.usage=usage,.sharingMode=VK_SHARING_MODE_EXCLUSIVE};
    CHECK(vkCreateImage(device,&ci,NULL,image));VkMemoryRequirements req;vkGetImageMemoryRequirements(device,*image,&req);
    uint32_t type=memory_type(physical,req.memoryTypeBits,VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    if(type==UINT32_MAX)return 1;
    VkMemoryAllocateInfo alloc={.sType=VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,.allocationSize=req.size,.memoryTypeIndex=type};
    CHECK(vkAllocateMemory(device,&alloc,NULL,memory));CHECK(vkBindImageMemory(device,*image,*memory,0));
    VkImageViewCreateInfo vi={.sType=VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,.image=*image,.viewType=VK_IMAGE_VIEW_TYPE_2D,
        .format=format,.subresourceRange={aspect,0,1,0,1}};CHECK(vkCreateImageView(device,&vi,NULL,view));return 0;
}
struct HleRenderer {
    VkInstance instance;
    VkPhysicalDevice physical;
    VkDevice device;
    VkQueue queue;
    VkImage images[2];
    VkDeviceMemory memories[2];
    VkImageView views[2];
    VkRenderPass pass;
    VkFramebuffer framebuffer;
    VkImage texture;
    VkDeviceMemory texture_memory;
    VkImageView texture_view;
    VkSampler sampler;
    VkDescriptorSetLayout set_layout;
    VkDescriptorPool descriptors;
    VkDescriptorSet descriptor;
    VkPipelineLayout layout;
    VkShaderModule shaders[2];
    VkPipeline pipeline;
    VkPipeline no_depth_pipeline;
    VkPipeline overlay_pipeline;
    VkPipeline scene_pipelines[13][8];
    size_t vertex_capacity;
    size_t vertex_start;
    unsigned texture_size;
    unsigned char *atlas_shadow;
    VkBuffer buffer;
    VkDeviceMemory memory;
    void * mapped;
    VkCommandPool pool;
    VkCommandBuffer cmd;
    VkFence fence;
    unsigned frames; int failed;
};
static int renderer_create(HleRenderer *,const HleVertex *,size_t,const unsigned char *);
static int renderer_draw(HleRenderer *,const HleVertex *,size_t,const unsigned char *,const HleMaterial *,const unsigned char *,unsigned char *,int,unsigned);
void nps64_hle_renderer_release(HleRenderer *r) {
    if(!r)return;
#ifdef __ORBIS__
    /* PS4 teardown is still under investigation. Frontend owns one reusable
     * renderer for process lifetime; never create another on ROM changes. */
    REPORT("[hle-game] renderer retained until process exit\n");
    return;
#else
    if(!r->device){if(r->instance)vkDestroyInstance(r->instance,NULL);free(r->atlas_shadow);free(r);return;}
    vkDeviceWaitIdle(r->device);
    if(r->fence)vkDestroyFence(r->device,r->fence,NULL);
    if(r->pool)vkDestroyCommandPool(r->device,r->pool,NULL);
    if(r->mapped)vkUnmapMemory(r->device,r->memory);
    if(r->buffer)vkDestroyBuffer(r->device,r->buffer,NULL);
    if(r->memory)vkFreeMemory(r->device,r->memory,NULL);
    for(unsigned k=0;k<13;k++)for(unsigned i=0;i<8;i++)if(r->scene_pipelines[k][i])vkDestroyPipeline(r->device,r->scene_pipelines[k][i],NULL);
    if(r->pipeline)vkDestroyPipeline(r->device,r->pipeline,NULL);
    if(r->no_depth_pipeline)vkDestroyPipeline(r->device,r->no_depth_pipeline,NULL);
    if(r->overlay_pipeline)vkDestroyPipeline(r->device,r->overlay_pipeline,NULL);
    for(unsigned i=0;i<2;i++)if(r->shaders[i])vkDestroyShaderModule(r->device,r->shaders[i],NULL);
    if(r->layout)vkDestroyPipelineLayout(r->device,r->layout,NULL);
    if(r->descriptors)vkDestroyDescriptorPool(r->device,r->descriptors,NULL);
    if(r->set_layout)vkDestroyDescriptorSetLayout(r->device,r->set_layout,NULL);
    if(r->sampler)vkDestroySampler(r->device,r->sampler,NULL);
    if(r->texture_view)vkDestroyImageView(r->device,r->texture_view,NULL);
    if(r->texture)vkDestroyImage(r->device,r->texture,NULL);
    if(r->texture_memory)vkFreeMemory(r->device,r->texture_memory,NULL);
    if(r->framebuffer)vkDestroyFramebuffer(r->device,r->framebuffer,NULL);
    if(r->pass)vkDestroyRenderPass(r->device,r->pass,NULL);
    for(unsigned i=0;i<2;i++){
        if(r->views[i])vkDestroyImageView(r->device,r->views[i],NULL);
        if(r->images[i])vkDestroyImage(r->device,r->images[i],NULL);
        if(r->memories[i])vkFreeMemory(r->device,r->memories[i],NULL);
    }
    vkDestroyDevice(r->device,NULL);vkDestroyInstance(r->instance,NULL);free(r->atlas_shadow);free(r);
#endif
}
int nps64_hle_renderer_open(HleRenderer **renderer,const HleSceneFrame *scene) {
    if(!renderer||*renderer||!scene||!scene->vertices||!scene->depth||!scene->materials||!scene->atlas||!scene->count||scene->count>30000||scene->count%3)return 1;
    HleRenderer *r=calloc(1,sizeof(*r));if(!r)return 1;
    *renderer=r;
    if(renderer_create(r,scene->vertices,scene->count,scene->atlas)){r->failed=1;return 1;}
    REPORT("[hle-game] persistent renderer ready; capacity=30000 atlas=512\n");return 0;
}
int nps64_hle_renderer_frame(HleRenderer *r,const HleSceneFrame *scene,unsigned char *pixels) {
    if(!r||r->failed||!scene||!pixels||!scene->vertices||!scene->depth||!scene->materials||!scene->atlas||scene->count>r->vertex_capacity||scene->count%3)return 1;
    if(renderer_draw(r,scene->vertices,scene->count,scene->depth,scene->materials,scene->atlas,pixels,1,r->frames)){r->failed=1;return 1;}
    return 0;
}
static int renderer_create(HleRenderer *r,const HleVertex *draw_vertices,size_t draw_count,const unsigned char *atlas) {
    r->texture_size=atlas?HLE_ATLAS_SIZE:256;
    r->vertex_start=(size_t)r->texture_size*r->texture_size*4;
#ifdef __ORBIS__
    if(!hle_load_global())return 1;
#endif
    VkApplicationInfo app={.sType=VK_STRUCTURE_TYPE_APPLICATION_INFO,.pApplicationName="NPS64 HLE graphics probe",.apiVersion=VK_API_VERSION_1_1};
    VkInstanceCreateInfo ici={.sType=VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,.pApplicationInfo=&app};
    CHECK(vkCreateInstance(&ici,NULL,&r->instance));
#ifdef __ORBIS__
    if(!hle_load_instance(r->instance))return 1;
#endif
    uint32_t count=0;CHECK(vkEnumeratePhysicalDevices(r->instance,&count,NULL));if(!count)return 1;
    VkPhysicalDevice *devices=calloc(count,sizeof(*devices));if(!devices)return 1;
    CHECK(vkEnumeratePhysicalDevices(r->instance,&count,devices));r->physical=devices[0];free(devices);
    VkPhysicalDeviceProperties props;vkGetPhysicalDeviceProperties(r->physical,&props);
    REPORT("[hle-probe] graphics device=%s; no paraLLEl-RDP\n",props.deviceName);
    vkGetPhysicalDeviceQueueFamilyProperties(r->physical,&count,NULL);
    VkQueueFamilyProperties *families=calloc(count,sizeof(*families));if(!families)return 1;
    vkGetPhysicalDeviceQueueFamilyProperties(r->physical,&count,families);uint32_t family=0;
    while(family<count&&!(families[family].queueFlags&VK_QUEUE_GRAPHICS_BIT))family++;
    free(families);if(family==count)return 1;float priority=1;
    VkDeviceQueueCreateInfo qci={.sType=VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,.queueFamilyIndex=family,.queueCount=1,.pQueuePriorities=&priority};
    VkDeviceCreateInfo dci={.sType=VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,.queueCreateInfoCount=1,.pQueueCreateInfos=&qci};
    CHECK(vkCreateDevice(r->physical,&dci,NULL,&r->device));
#ifdef __ORBIS__
    if(!hle_load_device(r->device))return 1;
#endif
    vkGetDeviceQueue(r->device,family,0,&r->queue);
    
    if(image_create(r->device,r->physical,VK_FORMAT_R8G8B8A8_UNORM,VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT|VK_IMAGE_USAGE_TRANSFER_SRC_BIT,VK_IMAGE_ASPECT_COLOR_BIT,256,&r->images[0],&r->memories[0],&r->views[0]))return 1;
    if(image_create(r->device,r->physical,VK_FORMAT_D32_SFLOAT,VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,VK_IMAGE_ASPECT_DEPTH_BIT,256,&r->images[1],&r->memories[1],&r->views[1]))return 1;
    VkAttachmentDescription attachments[2]={
        {.format=VK_FORMAT_R8G8B8A8_UNORM,.samples=VK_SAMPLE_COUNT_1_BIT,.loadOp=VK_ATTACHMENT_LOAD_OP_CLEAR,.storeOp=VK_ATTACHMENT_STORE_OP_STORE,.stencilLoadOp=VK_ATTACHMENT_LOAD_OP_DONT_CARE,.stencilStoreOp=VK_ATTACHMENT_STORE_OP_DONT_CARE,.initialLayout=VK_IMAGE_LAYOUT_UNDEFINED,.finalLayout=VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL},
        {.format=VK_FORMAT_D32_SFLOAT,.samples=VK_SAMPLE_COUNT_1_BIT,.loadOp=VK_ATTACHMENT_LOAD_OP_CLEAR,.storeOp=VK_ATTACHMENT_STORE_OP_DONT_CARE,.stencilLoadOp=VK_ATTACHMENT_LOAD_OP_DONT_CARE,.stencilStoreOp=VK_ATTACHMENT_STORE_OP_DONT_CARE,.initialLayout=VK_IMAGE_LAYOUT_UNDEFINED,.finalLayout=VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL}};
    VkAttachmentReference color={0,VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL},depth={1,VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};
    VkSubpassDescription subpass={.pipelineBindPoint=VK_PIPELINE_BIND_POINT_GRAPHICS,.colorAttachmentCount=1,.pColorAttachments=&color,.pDepthStencilAttachment=&depth};
    VkSubpassDependency deps[2]={
        {.srcSubpass=VK_SUBPASS_EXTERNAL,.dstSubpass=0,.srcStageMask=VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,.dstStageMask=VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT|VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,.dstAccessMask=VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT|VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT},
        {.srcSubpass=0,.dstSubpass=VK_SUBPASS_EXTERNAL,.srcStageMask=VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,.dstStageMask=VK_PIPELINE_STAGE_TRANSFER_BIT,.srcAccessMask=VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,.dstAccessMask=VK_ACCESS_TRANSFER_READ_BIT}};
    VkRenderPassCreateInfo rpci={.sType=VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,.attachmentCount=2,.pAttachments=attachments,.subpassCount=1,.pSubpasses=&subpass,.dependencyCount=2,.pDependencies=deps};
    CHECK(vkCreateRenderPass(r->device,&rpci,NULL,&r->pass));
    VkFramebufferCreateInfo fbci={.sType=VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,.renderPass=r->pass,.attachmentCount=2,.pAttachments=r->views,.width=256,.height=256,.layers=1};
    CHECK(vkCreateFramebuffer(r->device,&fbci,NULL,&r->framebuffer));
    
    if(image_create(r->device,r->physical,VK_FORMAT_R8G8B8A8_UNORM,VK_IMAGE_USAGE_SAMPLED_BIT|VK_IMAGE_USAGE_TRANSFER_DST_BIT|VK_IMAGE_USAGE_TRANSFER_SRC_BIT,VK_IMAGE_ASPECT_COLOR_BIT,r->texture_size,&r->texture,&r->texture_memory,&r->texture_view))return 1;
    VkSamplerCreateInfo sampler_ci={.sType=VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,.magFilter=VK_FILTER_NEAREST,.minFilter=VK_FILTER_NEAREST,.mipmapMode=VK_SAMPLER_MIPMAP_MODE_NEAREST,.addressModeU=VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,.addressModeV=VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,.addressModeW=VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE};
    CHECK(vkCreateSampler(r->device,&sampler_ci,NULL,&r->sampler));
    VkDescriptorSetLayoutBinding binding={.binding=0,.descriptorType=VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,.descriptorCount=1,.stageFlags=VK_SHADER_STAGE_FRAGMENT_BIT};
    VkDescriptorSetLayoutCreateInfo set_ci={.sType=VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,.bindingCount=1,.pBindings=&binding};
    CHECK(vkCreateDescriptorSetLayout(r->device,&set_ci,NULL,&r->set_layout));
    VkDescriptorPoolSize pool_size={VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,1};
    VkDescriptorPoolCreateInfo descriptor_ci={.sType=VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,.maxSets=1,.poolSizeCount=1,.pPoolSizes=&pool_size};
    CHECK(vkCreateDescriptorPool(r->device,&descriptor_ci,NULL,&r->descriptors));
    VkDescriptorSetAllocateInfo set_alloc={.sType=VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,.descriptorPool=r->descriptors,.descriptorSetCount=1,.pSetLayouts=&r->set_layout};
    CHECK(vkAllocateDescriptorSets(r->device,&set_alloc,&r->descriptor));
    VkDescriptorImageInfo image_info={r->sampler,r->texture_view,VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
    VkWriteDescriptorSet write={.sType=VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,.dstSet=r->descriptor,.dstBinding=0,.descriptorCount=1,.descriptorType=VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,.pImageInfo=&image_info};
    vkUpdateDescriptorSets(r->device,1,&write,0,NULL);
    VkPushConstantRange push={VK_SHADER_STAGE_VERTEX_BIT|VK_SHADER_STAGE_FRAGMENT_BIT,0,sizeof(ProbeDraw)};
    VkPipelineLayoutCreateInfo plci={.sType=VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,.setLayoutCount=1,.pSetLayouts=&r->set_layout,.pushConstantRangeCount=1,.pPushConstantRanges=&push};
    CHECK(vkCreatePipelineLayout(r->device,&plci,NULL,&r->layout));
    VkShaderModuleCreateInfo smci={.sType=VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,.codeSize=sizeof(hle_vert),.pCode=hle_vert};
    CHECK(vkCreateShaderModule(r->device,&smci,NULL,&r->shaders[0]));smci.codeSize=sizeof(hle_frag);smci.pCode=hle_frag;CHECK(vkCreateShaderModule(r->device,&smci,NULL,&r->shaders[1]));
    VkPipelineShaderStageCreateInfo stages[2]={
        {.sType=VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,.stage=VK_SHADER_STAGE_VERTEX_BIT,.module=r->shaders[0],.pName="main"},
        {.sType=VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,.stage=VK_SHADER_STAGE_FRAGMENT_BIT,.module=r->shaders[1],.pName="main"}};
    VkVertexInputBindingDescription vertex_binding={0,sizeof(HleVertex),VK_VERTEX_INPUT_RATE_VERTEX};
    VkVertexInputAttributeDescription attributes[3]={
        {0,0,VK_FORMAT_R32G32B32A32_SFLOAT,offsetof(HleVertex,position)},
        {1,0,VK_FORMAT_R32G32B32A32_SFLOAT,offsetof(HleVertex,color)},
        {2,0,VK_FORMAT_R32G32_SFLOAT,offsetof(HleVertex,uv)}};
    VkPipelineVertexInputStateCreateInfo vertex={.sType=VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,.vertexBindingDescriptionCount=1,.pVertexBindingDescriptions=&vertex_binding,.vertexAttributeDescriptionCount=3,.pVertexAttributeDescriptions=attributes};
    VkPipelineInputAssemblyStateCreateInfo assembly={.sType=VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,.topology=VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST};
    VkViewport viewport={0,0,256,256,0,1};VkRect2D scissor={{0,0},{256,256}};
    VkPipelineViewportStateCreateInfo vp={.sType=VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,.viewportCount=1,.pViewports=&viewport,.scissorCount=1,.pScissors=&scissor};
    VkPipelineRasterizationStateCreateInfo raster={.sType=VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,.polygonMode=VK_POLYGON_MODE_FILL,.cullMode=VK_CULL_MODE_NONE,.frontFace=VK_FRONT_FACE_COUNTER_CLOCKWISE,.lineWidth=1};
    VkPipelineMultisampleStateCreateInfo ms={.sType=VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,.rasterizationSamples=VK_SAMPLE_COUNT_1_BIT};
    VkPipelineDepthStencilStateCreateInfo ds={.sType=VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,.depthTestEnable=VK_TRUE,.depthWriteEnable=VK_TRUE,.depthCompareOp=VK_COMPARE_OP_LESS};
    VkPipelineColorBlendAttachmentState blend={.colorWriteMask=15};
    VkPipelineColorBlendStateCreateInfo cb={.sType=VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,.attachmentCount=1,.pAttachments=&blend};
    VkGraphicsPipelineCreateInfo gpci={.sType=VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,.stageCount=2,.pStages=stages,.pVertexInputState=&vertex,.pInputAssemblyState=&assembly,.pViewportState=&vp,.pRasterizationState=&raster,.pMultisampleState=&ms,.pDepthStencilState=&ds,.pColorBlendState=&cb,.layout=r->layout,.renderPass=r->pass};
    CHECK(vkCreateGraphicsPipelines(r->device,VK_NULL_HANDLE,1,&gpci,NULL,&r->pipeline));
    
    ds.depthTestEnable=VK_FALSE;ds.depthWriteEnable=VK_FALSE;
    CHECK(vkCreateGraphicsPipelines(r->device,VK_NULL_HANDLE,1,&gpci,NULL,&r->no_depth_pipeline));
    ds.depthTestEnable=VK_TRUE;ds.depthWriteEnable=VK_TRUE;
    
    blend.blendEnable=VK_TRUE;blend.srcColorBlendFactor=VK_BLEND_FACTOR_SRC_ALPHA;blend.dstColorBlendFactor=VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;blend.colorBlendOp=VK_BLEND_OP_ADD;
    blend.srcAlphaBlendFactor=VK_BLEND_FACTOR_ONE;blend.dstAlphaBlendFactor=VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;blend.alphaBlendOp=VK_BLEND_OP_ADD;
    ds.depthWriteEnable=VK_FALSE;ds.depthCompareOp=VK_COMPARE_OP_ALWAYS;
    scissor.offset.x=128;scissor.extent.width=128;
    CHECK(vkCreateGraphicsPipelines(r->device,VK_NULL_HANDLE,1,&gpci,NULL,&r->overlay_pipeline));
    VkDynamicState dynamic_scissor=VK_DYNAMIC_STATE_SCISSOR;
    VkPipelineDynamicStateCreateInfo dynamic={.sType=VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,.dynamicStateCount=1,.pDynamicStates=&dynamic_scissor};
    gpci.pDynamicState=&dynamic;
    scissor.offset.x=0;scissor.extent.width=256;
    VkSpecializationMapEntry material_entry={.constantID=0,.offset=0,.size=sizeof(uint32_t)};
    uint32_t material_kind=0;
    VkSpecializationInfo material_specialization={.mapEntryCount=1,.pMapEntries=&material_entry,.dataSize=sizeof(material_kind),.pData=&material_kind};
    stages[1].pSpecializationInfo=&material_specialization;
    for(material_kind=0;material_kind<13;material_kind++)for(unsigned i=0;i<8;i++){
        ds.depthTestEnable=(i&1)?VK_TRUE:VK_FALSE;ds.depthWriteEnable=(i&2)?VK_TRUE:VK_FALSE;
        ds.depthCompareOp=(i&1)?VK_COMPARE_OP_LESS:VK_COMPARE_OP_ALWAYS;
        /* Depth writes require an enabled depth test in Vulkan. */
        if(i&2)ds.depthTestEnable=VK_TRUE;
        blend.blendEnable=(i&4)?VK_TRUE:VK_FALSE;
        CHECK(vkCreateGraphicsPipelines(r->device,VK_NULL_HANDLE,1,&gpci,NULL,&r->scene_pipelines[material_kind][i]));
    }
    r->vertex_capacity=30000;
    VkBufferCreateInfo bci={.sType=VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,.size=r->vertex_start+r->vertex_capacity*sizeof(HleVertex),.usage=VK_BUFFER_USAGE_TRANSFER_DST_BIT|VK_BUFFER_USAGE_TRANSFER_SRC_BIT|VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,.sharingMode=VK_SHARING_MODE_EXCLUSIVE};
    CHECK(vkCreateBuffer(r->device,&bci,NULL,&r->buffer));VkMemoryRequirements req;vkGetBufferMemoryRequirements(r->device,r->buffer,&req);
    uint32_t type=memory_type(r->physical,req.memoryTypeBits,VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);if(type==UINT32_MAX)return 1;
    VkMemoryAllocateInfo alloc={.sType=VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,.allocationSize=req.size,.memoryTypeIndex=type};
    CHECK(vkAllocateMemory(r->device,&alloc,NULL,&r->memory));CHECK(vkBindBufferMemory(r->device,r->buffer,r->memory,0));CHECK(vkMapMemory(r->device,r->memory,0,VK_WHOLE_SIZE,0,&r->mapped));
    VkCommandPoolCreateInfo cpci={.sType=VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,.flags=VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,.queueFamilyIndex=family};CHECK(vkCreateCommandPool(r->device,&cpci,NULL,&r->pool));
    VkCommandBufferAllocateInfo cba={.sType=VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,.commandPool=r->pool,.level=VK_COMMAND_BUFFER_LEVEL_PRIMARY,.commandBufferCount=1};CHECK(vkAllocateCommandBuffers(r->device,&cba,&r->cmd));
    VkFenceCreateInfo fci={.sType=VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};CHECK(vkCreateFence(r->device,&fci,NULL,&r->fence));
    memcpy((unsigned char*)r->mapped+r->vertex_start,draw_vertices,draw_count*sizeof(HleVertex));
    REPORT("[hle-probe] decoded N64 vertices uploaded to Vulkan vertex buffer\n");
    /* Reuse the coherent readback allocation for the initial r->texture upload. */
    unsigned char *texels=r->mapped;
    for(unsigned y=0;y<256;y++)for(unsigned x=0;x<256;x++) {
        unsigned p=(y*256+x)*4;texels[p]=255;texels[p+1]=((x/32+y/32)&1)?255:0;texels[p+2]=0;texels[p+3]=255;
    }
    if(atlas)memcpy(r->mapped,atlas,r->vertex_start);
    VkCommandBufferBeginInfo upload_begin={.sType=VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};CHECK(vkBeginCommandBuffer(r->cmd,&upload_begin));
    VkImageMemoryBarrier texture_barrier={.sType=VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,.dstAccessMask=VK_ACCESS_TRANSFER_WRITE_BIT,.oldLayout=VK_IMAGE_LAYOUT_UNDEFINED,.newLayout=VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,.srcQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED,.dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED,.image=r->texture,.subresourceRange={VK_IMAGE_ASPECT_COLOR_BIT,0,1,0,1}};
    vkCmdPipelineBarrier(r->cmd,VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT,0,0,NULL,0,NULL,1,&texture_barrier);
    VkBufferImageCopy texture_copy={.imageSubresource={VK_IMAGE_ASPECT_COLOR_BIT,0,0,1},.imageExtent={r->texture_size,r->texture_size,1}};
    vkCmdCopyBufferToImage(r->cmd,r->buffer,r->texture,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,1,&texture_copy);
    texture_barrier.srcAccessMask=VK_ACCESS_TRANSFER_WRITE_BIT;texture_barrier.dstAccessMask=VK_ACCESS_SHADER_READ_BIT;texture_barrier.oldLayout=VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;texture_barrier.newLayout=VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    vkCmdPipelineBarrier(r->cmd,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,0,0,NULL,0,NULL,1,&texture_barrier);
    CHECK(vkEndCommandBuffer(r->cmd));VkSubmitInfo upload={.sType=VK_STRUCTURE_TYPE_SUBMIT_INFO,.commandBufferCount=1,.pCommandBuffers=&r->cmd};
    REPORT("[hle-probe] texture upload begin\n");CHECK(vkQueueSubmit(r->queue,1,&upload,r->fence));CHECK(vkWaitForFences(r->device,1,&r->fence,VK_TRUE,5000000000ull));CHECK(vkResetFences(r->device,1,&r->fence));
    REPORT("[hle-probe] texture upload completed\n");
    if(atlas){
        /* Check the actual optimal-tiled image, not just CPU staging bytes. */
        CHECK(vkResetCommandBuffer(r->cmd,0));CHECK(vkBeginCommandBuffer(r->cmd,&upload_begin));
        texture_barrier.srcAccessMask=VK_ACCESS_SHADER_READ_BIT;texture_barrier.dstAccessMask=VK_ACCESS_TRANSFER_READ_BIT;
        texture_barrier.oldLayout=VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;texture_barrier.newLayout=VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        vkCmdPipelineBarrier(r->cmd,VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT,0,0,NULL,0,NULL,1,&texture_barrier);
        vkCmdCopyImageToBuffer(r->cmd,r->texture,VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,r->buffer,1,&texture_copy);
        VkBufferMemoryBarrier host={.sType=VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,.srcAccessMask=VK_ACCESS_TRANSFER_WRITE_BIT,.dstAccessMask=VK_ACCESS_HOST_READ_BIT,
            .srcQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED,.dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED,.buffer=r->buffer,.offset=0,.size=r->vertex_start};
        vkCmdPipelineBarrier(r->cmd,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_HOST_BIT,0,0,NULL,1,&host,0,NULL);
        texture_barrier.srcAccessMask=VK_ACCESS_TRANSFER_READ_BIT;texture_barrier.dstAccessMask=VK_ACCESS_SHADER_READ_BIT;
        texture_barrier.oldLayout=VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;texture_barrier.newLayout=VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        vkCmdPipelineBarrier(r->cmd,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,0,0,NULL,0,NULL,1,&texture_barrier);
        CHECK(vkEndCommandBuffer(r->cmd));CHECK(vkQueueSubmit(r->queue,1,&upload,r->fence));CHECK(vkWaitForFences(r->device,1,&r->fence,VK_TRUE,5000000000ull));CHECK(vkResetFences(r->device,1,&r->fence));
        size_t mismatches=0;unsigned colored=0;
        for(size_t i=0;i<r->vertex_start;i++)mismatches+=((const unsigned char *)r->mapped)[i]!=atlas[i];
        for(size_t i=0;i<r->vertex_start;i+=4)colored+=atlas[i]!=atlas[i+1]||atlas[i+1]!=atlas[i+2];
        REPORT("[hle-game] atlas GPU readback bytes=%zu mismatches=%zu colored_texels=%u; specialized material pipelines\n",r->vertex_start,mismatches,colored);
        if(mismatches){REPORT("[hle-game] FAIL GPU atlas differs from captured texture data\n");return 1;}
        r->atlas_shadow=malloc(r->vertex_start);if(!r->atlas_shadow)return 1;
        memcpy(r->atlas_shadow,atlas,r->vertex_start);
    }
    return 0;
}
static int renderer_draw(HleRenderer *r,const HleVertex *draw_vertices,size_t draw_count,const unsigned char *game_depth,const HleMaterial *materials,const unsigned char *atlas,unsigned char *pixels,int game_scene,unsigned frame) {
    memcpy((unsigned char *)r->mapped+r->vertex_start,draw_vertices,draw_count*sizeof(HleVertex));
    VkBufferImageCopy dirty[64];unsigned dirty_count=0;
    if(atlas){
        for(unsigned ty=0;ty<8;ty++)for(unsigned tx=0;tx<8;tx++){
            unsigned changed=0;
            for(unsigned y=0;y<64&&!changed;y++){
                size_t offset=((ty*64+y)*512+tx*64)*4;
                changed=memcmp(r->atlas_shadow+offset,atlas+offset,64*4)!=0;
            }
            if(!changed)continue;
            for(unsigned y=0;y<64;y++){
                size_t offset=((ty*64+y)*512+tx*64)*4;
                memcpy((unsigned char *)r->mapped+offset,atlas+offset,64*4);
                memcpy(r->atlas_shadow+offset,atlas+offset,64*4);
            }
            dirty[dirty_count++]=(VkBufferImageCopy){
                .bufferOffset=(ty*64*512+tx*64)*4,.bufferRowLength=512,.bufferImageHeight=512,
                .imageSubresource={VK_IMAGE_ASPECT_COLOR_BIT,0,0,1},
                .imageOffset={(int)(tx*64),(int)(ty*64),0},.imageExtent={64,64,1}};
        }
    }
    /* Full tiled uploads are the PS4 baseline while partial transfers are audited. */
    int full_upload=0;
#ifdef __ORBIS__
    full_upload=1;
#else
    full_upload=getenv("NPS64_FULL_ATLAS_UPLOAD")!=NULL;
#endif
    if(atlas&&full_upload&&dirty_count){
        memcpy(r->mapped,atlas,r->vertex_start);dirty_count=1;
        dirty[0]=(VkBufferImageCopy){.imageSubresource={VK_IMAGE_ASPECT_COLOR_BIT,0,0,1},.imageExtent={512,512,1}};
    }
    VkImageMemoryBarrier texture_barrier={.sType=VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,.srcQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED,.dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED,.image=r->texture,.subresourceRange={VK_IMAGE_ASPECT_COLOR_BIT,0,1,0,1}};
        CHECK(vkResetCommandBuffer(r->cmd,0));VkCommandBufferBeginInfo begin={.sType=VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};CHECK(vkBeginCommandBuffer(r->cmd,&begin));
        if(dirty_count){
            texture_barrier.srcAccessMask=VK_ACCESS_SHADER_READ_BIT;texture_barrier.dstAccessMask=VK_ACCESS_TRANSFER_WRITE_BIT;
            texture_barrier.oldLayout=VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;texture_barrier.newLayout=VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            vkCmdPipelineBarrier(r->cmd,VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT,0,0,NULL,0,NULL,1,&texture_barrier);
            vkCmdCopyBufferToImage(r->cmd,r->buffer,r->texture,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,dirty_count,dirty);
            texture_barrier.srcAccessMask=VK_ACCESS_TRANSFER_WRITE_BIT;texture_barrier.dstAccessMask=VK_ACCESS_SHADER_READ_BIT;
            texture_barrier.oldLayout=VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;texture_barrier.newLayout=VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            vkCmdPipelineBarrier(r->cmd,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,0,0,NULL,0,NULL,1,&texture_barrier);
        }
        VkClearValue clear[2]={ {.color={{0.0f,0.0f,0.0f,1.0f}}},{.depthStencil={1.0f,0}}};
        VkRenderPassBeginInfo rp={.sType=VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,.renderPass=r->pass,.framebuffer=r->framebuffer,.renderArea={{0,0},{256,256}},.clearValueCount=2,.pClearValues=clear};
        vkCmdBeginRenderPass(r->cmd,&rp,VK_SUBPASS_CONTENTS_INLINE);vkCmdBindPipeline(r->cmd,VK_PIPELINE_BIND_POINT_GRAPHICS,r->pipeline);
        VkDeviceSize vertex_offset=r->vertex_start;vkCmdBindVertexBuffers(r->cmd,0,1,&r->buffer,&vertex_offset);
        vkCmdBindDescriptorSets(r->cmd,VK_PIPELINE_BIND_POINT_GRAPHICS,r->layout,0,1,&r->descriptor,0,NULL);
        ProbeDraw foreground={.color={1,game_scene||frame>=64?1:0,game_scene||frame>=64?1:0,1},.depth=game_scene?0:-0.3f,.textured=!game_scene&&frame>=64},background={.color={0,0,1,1},.depth=0.3f};
        vkCmdPushConstants(r->cmd,r->layout,VK_SHADER_STAGE_VERTEX_BIT|VK_SHADER_STAGE_FRAGMENT_BIT,0,sizeof(foreground),&foreground);
        if(game_scene) {
            /* Batch consecutive triangles with the same depth state, preserving order. */
            for(size_t first=0;first<draw_count;) {
                size_t end=first+3;unsigned enabled=game_depth[first/3]!=0;
                while(end<draw_count&&(game_depth[end/3]!=0)==enabled&&(!materials||!memcmp(&materials[first/3],&materials[end/3],sizeof(HleMaterial))))end+=3;
                foreground.textured=materials&&atlas&&materials[first/3].rect[2]>0?2:0;
                if(foreground.textured){foreground.wrap=materials[first/3].wrap;memcpy(foreground.rect,materials[first/3].rect,sizeof(foreground.rect));}
                unsigned state=enabled?3:0;
                if(materials){
                    const HleMaterial *m=&materials[first/3];memcpy(foreground.prim,m->prim,sizeof(foreground.prim));memcpy(foreground.env,m->env,sizeof(foreground.env));memcpy(foreground.mux,m->mux,sizeof(foreground.mux));memcpy(foreground.sampling,m->sampling,sizeof(foreground.sampling));
                    foreground.padding=(m->mux[2]&HLE_MATERIAL_VALID)&&!(m->mux[2]&HLE_TWO_CYCLE)&&
                        (m->mux[0]&0xffffffu)==0x30b261u&&m->mux[1]==0x44664924u?1:0;
                    if((m->mux[2]&HLE_MATERIAL_VALID)&&!(m->mux[2]&HLE_TWO_CYCLE)){
                        if((m->mux[0]&0xffffffu)==0x127e24u&&m->mux[1]==0xfffff9fcu)foreground.padding=2;
                        if((m->mux[0]&0xffffffu)==0xffffffu&&m->mux[1]==0xfffe793cu)foreground.padding=3;
                        if((m->mux[0]&0xffffffu)==0x121824u&&m->mux[1]==0xff33ffffu)foreground.padding=4;
                        if((m->mux[0]&0xffffffu)==0xffffffu&&m->mux[1]==0xfffe7b3du)foreground.padding=5;
                        if((m->mux[0]&0xffffffu)==0x147e28u&&m->mux[1]==0x44fe7b3du)foreground.padding=6;
                        if((m->mux[0]&0xffffffu)==0xffffffu&&m->mux[1]==0xfffcfa7du)foreground.padding=7;
                        if((m->mux[0]&0xffffffu)==0xffffffu&&m->mux[1]==0xfffcf279u)foreground.padding=8;
                        if((m->mux[0]&0xffffffu)==0xffffffu&&m->mux[1]==0xfffcf87cu)foreground.padding=9;
                        if((m->mux[0]&0xffffffu)==0x127e24u&&m->mux[1]==0xfffff3f9u)foreground.padding=10;
                        if((m->mux[0]&0xffffffu)==31u&&m->mux[1]==0x001c0041u)foreground.padding=11;
                        if((m->mux[0]&0xffffffu)==31u&&m->mux[1]==0x001c00c3u)foreground.padding=12;
                    }
                    if(m->mux[2]&HLE_MATERIAL_VALID)state=((m->mux[2]&HLE_Z_COMPARE)?1:0)|((m->mux[2]&HLE_Z_WRITE)?2:0)|((m->mux[2]&(HLE_BLEND|HLE_ALPHA_COVERAGE))?4:0);
                }
                vkCmdPushConstants(r->cmd,r->layout,VK_SHADER_STAGE_VERTEX_BIT|VK_SHADER_STAGE_FRAGMENT_BIT,0,sizeof(foreground),&foreground);
                vkCmdBindPipeline(r->cmd,VK_PIPELINE_BIND_POINT_GRAPHICS,r->scene_pipelines[foreground.padding][state]);
                VkRect2D clip={{0,0},{256,256}};
                if(materials&&(materials[first/3].mux[2]&HLE_SCISSOR)){
                    const float *s=materials[first/3].scissor;int bounds[4];
                    for(unsigned i=0;i<4;i++){float x=s[i]*256;bounds[i]=x<0?0:x>256?256:(int)x;}
                    clip.offset.x=bounds[0];clip.offset.y=bounds[1];
                    clip.extent.width=bounds[2]>bounds[0]?(unsigned)(bounds[2]-bounds[0]):0;
                    clip.extent.height=bounds[3]>bounds[1]?(unsigned)(bounds[3]-bounds[1]):0;
                }
                vkCmdSetScissor(r->cmd,0,1,&clip);
                vkCmdDraw(r->cmd,(uint32_t)(end-first),1,(uint32_t)first,0);first=end;
            }
        }else vkCmdDraw(r->cmd,(uint32_t)draw_count,1,0,0);
        if(!game_scene){vkCmdPushConstants(r->cmd,r->layout,VK_SHADER_STAGE_VERTEX_BIT|VK_SHADER_STAGE_FRAGMENT_BIT,0,sizeof(background),&background);vkCmdDraw(r->cmd,3,1,0,0);}
        if(!game_scene&&frame>=64) {
            ProbeDraw overlay={.color={0,1,0,0.5f},.depth=-0.5f};
            vkCmdBindPipeline(r->cmd,VK_PIPELINE_BIND_POINT_GRAPHICS,r->overlay_pipeline);
            vkCmdPushConstants(r->cmd,r->layout,VK_SHADER_STAGE_VERTEX_BIT|VK_SHADER_STAGE_FRAGMENT_BIT,0,sizeof(overlay),&overlay);vkCmdDraw(r->cmd,3,1,0,0);
        }
        vkCmdEndRenderPass(r->cmd);
        /* After draw, vertex storage can serve as diagnostic atlas readback.
         * Every byte is compared, including tiles not visible in this frame. */
        int verify_atlas=atlas&&(frame<3||frame%120==0);
        if(verify_atlas){
            VkBufferMemoryBarrier scratch={.sType=VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
                .srcAccessMask=VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT,.dstAccessMask=VK_ACCESS_TRANSFER_WRITE_BIT,
                .srcQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED,.dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED,
                .buffer=r->buffer,.offset=r->vertex_start,.size=r->vertex_start};
            vkCmdPipelineBarrier(r->cmd,VK_PIPELINE_STAGE_VERTEX_INPUT_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT,0,0,NULL,1,&scratch,0,NULL);
            texture_barrier.srcAccessMask=VK_ACCESS_SHADER_READ_BIT;texture_barrier.dstAccessMask=VK_ACCESS_TRANSFER_READ_BIT;
            texture_barrier.oldLayout=VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;texture_barrier.newLayout=VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            vkCmdPipelineBarrier(r->cmd,VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT,0,0,NULL,0,NULL,1,&texture_barrier);
            VkBufferImageCopy check={.bufferOffset=r->vertex_start,.imageSubresource={VK_IMAGE_ASPECT_COLOR_BIT,0,0,1},.imageExtent={512,512,1}};
            vkCmdCopyImageToBuffer(r->cmd,r->texture,VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,r->buffer,1,&check);
            texture_barrier.srcAccessMask=VK_ACCESS_TRANSFER_READ_BIT;texture_barrier.dstAccessMask=VK_ACCESS_SHADER_READ_BIT;
            texture_barrier.oldLayout=VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;texture_barrier.newLayout=VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            vkCmdPipelineBarrier(r->cmd,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,0,0,NULL,0,NULL,1,&texture_barrier);
        }
        VkBufferImageCopy copy={.imageSubresource={VK_IMAGE_ASPECT_COLOR_BIT,0,0,1},.imageExtent={256,256,1}};vkCmdCopyImageToBuffer(r->cmd,r->images[0],VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,r->buffer,1,&copy);
        VkMemoryBarrier barrier={.sType=VK_STRUCTURE_TYPE_MEMORY_BARRIER,.srcAccessMask=VK_ACCESS_TRANSFER_WRITE_BIT,.dstAccessMask=VK_ACCESS_HOST_READ_BIT};
        vkCmdPipelineBarrier(r->cmd,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_HOST_BIT,0,1,&barrier,0,NULL,0,NULL);CHECK(vkEndCommandBuffer(r->cmd));
        VkSubmitInfo submit={.sType=VK_STRUCTURE_TYPE_SUBMIT_INFO,.commandBufferCount=1,.pCommandBuffers=&r->cmd};
        if(frame<3||frame%60==0)REPORT("[hle-probe] frame=%u submit graphics; atlas_dirty_cells=%u upload_bytes=%u\n",frame,dirty_count,dirty_count?(full_upload?512*512*4:dirty_count*64*64*4):0);CHECK(vkQueueSubmit(r->queue,1,&submit,r->fence));CHECK(vkWaitForFences(r->device,1,&r->fence,VK_TRUE,5000000000ull));
        memcpy(pixels,r->mapped,256*256*4);
        if(verify_atlas){
            const unsigned char *actual=(const unsigned char *)r->mapped+r->vertex_start;
            size_t mismatches=0,first=r->vertex_start;
            for(size_t i=0;i<r->vertex_start;i++)if(actual[i]!=atlas[i]){mismatches++;if(first==r->vertex_start)first=i;}
            REPORT("[hle-game] live atlas verify frame=%u mismatches=%zu first_byte=%zu transfer=%s\n",frame,mismatches,first,full_upload?"full":"incremental");
            if(mismatches)return 1;
        }
    CHECK(vkResetFences(r->device,1,&r->fence));r->frames++;return 0;
}
static int render_probe(const HleVertex *game_vertices,size_t game_count,const unsigned char *game_depth,const HleMaterial *materials,const unsigned char *atlas,const HleSequence *sequence,unsigned char pixels[256*256*4]) {
    int game_scene=game_vertices!=NULL;
    if(game_scene&&(!game_count||game_count>30000||game_count%3||!game_depth)){REPORT("[hle-game] invalid initial vertices=%zu (limit 30000), depth_present=%u\n",game_count,game_depth!=NULL);return 1;}
    uint8_t rdram[512];HleVertex decoded[3];
    if(!hle_display_list_fixture(rdram,sizeof(rdram)))return 1;
    HleDlInput dl_input={rdram,sizeof(rdram),0,256,256,64};
    HleDlOutput dl_output={.vertices=decoded,.capacity=3};
    HleDlStatus dl_status=hle_decode_f3dex2(&dl_input,0,&dl_output);
    REPORT("[hle-probe] F3DEX2 decode status=%u commands=%u triangles=%u vertices=%zu\n",(unsigned)dl_status,dl_output.commands,dl_output.triangles,dl_output.count);
    if(dl_status!=HLE_DL_OK||dl_output.count!=3)return 1;
    const HleVertex *draw_vertices=game_scene?game_vertices:decoded;
    size_t draw_count=game_scene?game_count:3;
    HleRenderer *r=calloc(1,sizeof(*r));if(!r)return 1;
    if(renderer_create(r,draw_vertices,draw_count,atlas)){nps64_hle_renderer_release(r);return 1;}
    static unsigned char coverage[256*256];
    static unsigned char reference[256*256*4];
    unsigned completed=0,changed=0,scene_frames=0,uniform_frames=0;
    for(unsigned frame=0;frame<(sequence?sequence->frames:game_scene?64u:128u);frame++) {
        if(sequence&&frame){
            HleSceneFrame next={0};int status=sequence->next(frame,&next,sequence->context);
            if(!status)break;if(status<0){REPORT("[hle-game] provider failed at frame=%u\n",frame);nps64_hle_renderer_release(r);return 1;}
            if(!next.vertices||!next.depth||!next.materials||!next.atlas||!next.count||next.count>r->vertex_capacity||next.count%3){REPORT("[hle-game] invalid frame=%u vertices=%zu capacity=%zu\n",frame,next.count,r->vertex_capacity);nps64_hle_renderer_release(r);return 1;}
            draw_vertices=next.vertices;draw_count=next.count;game_depth=next.depth;materials=next.materials;atlas=next.atlas;

        }
        if(renderer_draw(r,draw_vertices,draw_count,game_depth,materials,atlas,pixels,game_scene,frame)){nps64_hle_renderer_release(r);return 1;}
        unsigned center=(128*256+128)*4;

        if(sequence&&(frame==0||frame==sequence->frames-1)){
            unsigned lit=0,bright=0;for(unsigned i=0;i<65536;i++){
                const unsigned char *p=pixels+i*4;lit+=(p[0]|p[1]|p[2])!=0;bright+=p[0]>64||p[1]>64||p[2]>64;
            }
            REPORT("[hle-game] pixels frame=%u lit=%u bright=%u center=%u,%u,%u\n",frame,lit,bright,pixels[center],pixels[center+1],pixels[center+2]);
        }
        if(game_scene) {
            if(!frame||sequence) {
                unsigned lit=0,different=0;
                for(unsigned i=0;i<65536;i++) {
                    if(pixels[i*4]||pixels[i*4+1]||pixels[i*4+2])lit++;
                    if(memcmp(pixels+i*4,pixels,4))different++;
                }
                if(!frame)REPORT("[hle-game] Vulkan snapshot vertices=%zu visible_pixels=%u nonuniform_pixels=%u\n",draw_count,lit,different);
                if(!lit||!different){
                    if(!sequence){REPORT("[hle-game] FAIL uniform snapshot; not accepted as scene validation\n");nps64_hle_renderer_release(r);return 1;}
                    uniform_frames++;REPORT("[hle-game] uniform frame=%u; awaiting a visible scene\n",frame);
                }else scene_frames++;
            }
            if(frame&&memcmp(reference,pixels,sizeof(reference))){
                if(!sequence){REPORT("[hle-game] inconsistent repeated snapshot frame=%u\n",frame);nps64_hle_renderer_release(r);return 1;}
                changed++;
            }
            memcpy(reference,pixels,sizeof(reference));
            if(sequence&&sequence->present)sequence->present(frame,pixels,sequence->context);
        }else if(frame<64) {
            if(pixels[center]!=255||pixels[center+1]!=0||pixels[center+2]!=0||pixels[center+3]!=255){REPORT("[hle-probe] wrong depth output frame=%u\n",frame);nps64_hle_renderer_release(r);return 1;}
            for(unsigned i=0;i<65536;i++) {
                unsigned char *p=pixels+i*4;
                if(p[1]||p[2]||p[3]!=255||(p[0]!=0&&p[0]!=255)){REPORT("[hle-probe] invalid solid pixel frame=%u pixel=%u\n",frame,i);nps64_hle_renderer_release(r);return 1;}
                if(!frame)coverage[i]=p[0]!=0;
                else if(coverage[i]!=(p[0]!=0))return 1;
            }
        }else {
            for(unsigned i=0;i<65536;i++) {
                unsigned x=i%256,y=i/256;int red=0,g=0;
                if(coverage[i]){red=255;g=((x/32+y/32)&1)?255:0;if(x>=128){red=128;g=(g+255)/2;}}
                unsigned char *p=pixels+i*4;
                if(abs((int)p[0]-red)>1||abs((int)p[1]-g)>1||p[2]!=0||p[3]!=255){REPORT("[hle-probe] texture/blend mismatch frame=%u xy=%u,%u got=%u,%u,%u,%u expected=%d,%d,0,255\n",frame,x,y,p[0],p[1],p[2],p[3],red,g);nps64_hle_renderer_release(r);return 1;}
            }
        }
        if(frame==63&&!game_scene)REPORT("[hle-probe] PASS 64 graphics submissions, triangle coverage and depth occlusion\n");
        completed++;
    }
    int final_status=sequence&&!scene_frames;
    if(sequence){
        REPORT("[hle-game] %s Vulkan sequence frames=%u changed=%u; one device, reused vertex buffer and atlas\n",final_status?"FAIL":"PASS",completed,changed);
        REPORT("[hle-game] scene_frames=%u uniform_frames=%u; uniform frames do not validate scene rendering\n",scene_frames,uniform_frames);
    }
    else if(game_scene)REPORT("[hle-game] PASS real game geometry -> Vulkan: 64 repeated snapshots match\n");
    else {
        REPORT("[hle-probe] PASS 64 textured frames: upload, UV sampling, modulation, alpha blend, scissor and depth; all pixels checked\n");
        REPORT("[hle-probe] PASS F3DEX2 display list -> matrix/vertices -> Vulkan triangles\n");
    }
#ifdef __ORBIS__
    /* Keep this standalone probe's device alive through the result screen.
     * Isolate PS4 driver teardown from rasterization and presentation. The
     * process owns these resources until exit; native validation still exercises
     * the complete destruction path below. */
    REPORT("[hle-probe] PS4 device retained through result screen; returning pixels\n");
    return final_status;
#endif
    nps64_hle_renderer_release(r);return final_status;
}
int nps64_hle_graphics_probe(unsigned char *pixels){return render_probe(NULL,0,NULL,NULL,NULL,NULL,pixels);}
int nps64_hle_scene_probe(const HleVertex *vertices,size_t count,const unsigned char *depth,unsigned char *pixels){return render_probe(vertices,count,depth,NULL,NULL,NULL,pixels);}
int nps64_hle_textured_scene_probe(const HleVertex *vertices,size_t count,const unsigned char *depth,const HleMaterial *materials,const unsigned char *atlas,unsigned char *pixels){return render_probe(vertices,count,depth,materials,atlas,NULL,pixels);}
int nps64_hle_scene_sequence(const HleSequence *sequence,unsigned char *pixels){
    if(!sequence||!sequence->next||!sequence->frames||sequence->frames>120||!pixels)return 1;
    HleSceneFrame first={0};int status=sequence->next(0,&first,sequence->context);
    if(status!=1||!first.materials||!first.atlas||!first.vertices){REPORT("[hle-game] initial provider failed status=%d vertices=%zu materials=%u atlas=%u\n",status,first.count,first.materials!=NULL,first.atlas!=NULL);return 1;}
    return render_probe(first.vertices,first.count,first.depth,first.materials,first.atlas,sequence,pixels);
}
