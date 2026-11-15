/* Exact Q8 bilinear / nearest presentation on GPU, BGRA output for VideoOut.
 * SPDX-License-Identifier: GPL-2.0-or-later */
#include "gpu_presenter.h"
#include "gpu_present_spv.h"
#include "gpu_readback_memory.h"
#include "log.h"
#include <stdlib.h>
#define FUNCS(X) \
 X(CreateBuffer) X(DestroyBuffer) X(GetBufferMemoryRequirements) X(AllocateMemory) X(FreeMemory) \
 X(BindBufferMemory) X(MapMemory) X(UnmapMemory) X(CreateDescriptorSetLayout) X(DestroyDescriptorSetLayout) \
 X(CreateDescriptorPool) X(DestroyDescriptorPool) X(AllocateDescriptorSets) X(UpdateDescriptorSets) \
 X(CreatePipelineLayout) X(DestroyPipelineLayout) X(CreateShaderModule) X(DestroyShaderModule) \
 X(CreateComputePipelines) X(DestroyPipeline) X(CreateSampler) X(DestroySampler) \
 X(CmdBindPipeline) X(CmdBindDescriptorSets) X(CmdPushConstants) X(CmdDispatch) X(CmdPipelineBarrier)
struct Nps64Presenter {
 VkDevice device;VkBuffer buffers[3];VkDeviceMemory memory[3];void *pixels[3];
 VkDescriptorSetLayout set_layout;VkDescriptorPool pool;VkDescriptorSet sets[3];
 VkImageView previous_view[3];VkImageLayout previous_layout[3];
 VkPipelineLayout layout;VkPipeline pipeline;VkShaderModule shader;VkSampler sampler;
#define DECL(name) PFN_vk##name name;
 FUNCS(DECL)
#undef DECL
};
#define TRY(call) do{VkResult r=(call);if(r!=VK_SUCCESS){nps64_log("[gpu-present] %s failed=%d\n",#call,r);goto fail;}}while(0)
Nps64Presenter *nps64_presenter_create(VkPhysicalDevice gpu,VkDevice device,PFN_vkGetInstanceProcAddr gipa,VkInstance instance,PFN_vkGetDeviceProcAddr gdpa){
 Nps64Presenter *p=calloc(1,sizeof(*p));if(!p)return NULL;p->device=device;
#define LOAD(name) p->name=(PFN_vk##name)gdpa(device,"vk" #name);if(!p->name)goto fail;
 FUNCS(LOAD)
#undef LOAD
 PFN_vkGetPhysicalDeviceMemoryProperties get=(PFN_vkGetPhysicalDeviceMemoryProperties)gipa(instance,"vkGetPhysicalDeviceMemoryProperties");
 if(!get)goto fail;VkPhysicalDeviceMemoryProperties props;get(gpu,&props);
 for(unsigned i=0;i<3;i++){
  VkBufferCreateInfo b={.sType=VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,.size=1280*720*4,.usage=VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,.sharingMode=VK_SHARING_MODE_EXCLUSIVE};
  TRY(p->CreateBuffer(device,&b,NULL,&p->buffers[i]));VkMemoryRequirements req;p->GetBufferMemoryRequirements(device,p->buffers[i],&req);
  unsigned type=nps64_readback_memory_type(&props,req.memoryTypeBits);if(type==UINT32_MAX)goto fail;
  VkMemoryPropertyFlags required=VK_MEMORY_PROPERTY_HOST_COHERENT_BIT|VK_MEMORY_PROPERTY_HOST_CACHED_BIT;
  /* CPU HUD writes must be coherent; never silently use slow WC storage. */
  if((props.memoryTypes[type].propertyFlags&required)!=required)goto fail;
  VkMemoryAllocateInfo m={.sType=VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,.allocationSize=req.size,.memoryTypeIndex=type};
  TRY(p->AllocateMemory(device,&m,NULL,&p->memory[i]));TRY(p->BindBufferMemory(device,p->buffers[i],p->memory[i],0));TRY(p->MapMemory(device,p->memory[i],0,VK_WHOLE_SIZE,0,&p->pixels[i]));
 }
 VkDescriptorSetLayoutBinding bindings[2]={{.binding=0,.descriptorType=VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,.descriptorCount=1,.stageFlags=VK_SHADER_STAGE_COMPUTE_BIT},{.binding=1,.descriptorType=VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,.descriptorCount=1,.stageFlags=VK_SHADER_STAGE_COMPUTE_BIT}};
 VkDescriptorSetLayoutCreateInfo sl={.sType=VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,.bindingCount=2,.pBindings=bindings};TRY(p->CreateDescriptorSetLayout(device,&sl,NULL,&p->set_layout));
 VkDescriptorPoolSize sizes[2]={{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,3},{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,3}};
 VkDescriptorPoolCreateInfo dp={.sType=VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,.maxSets=3,.poolSizeCount=2,.pPoolSizes=sizes};TRY(p->CreateDescriptorPool(device,&dp,NULL,&p->pool));
 VkDescriptorSetLayout layouts[3]={p->set_layout,p->set_layout,p->set_layout};VkDescriptorSetAllocateInfo da={.sType=VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,.descriptorPool=p->pool,.descriptorSetCount=3,.pSetLayouts=layouts};TRY(p->AllocateDescriptorSets(device,&da,p->sets));
 VkPushConstantRange range={VK_SHADER_STAGE_COMPUTE_BIT,0,32};VkPipelineLayoutCreateInfo pl={.sType=VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,.setLayoutCount=1,.pSetLayouts=&p->set_layout,.pushConstantRangeCount=1,.pPushConstantRanges=&range};TRY(p->CreatePipelineLayout(device,&pl,NULL,&p->layout));
 VkShaderModuleCreateInfo sm={.sType=VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,.codeSize=sizeof nps64_present_spv,.pCode=nps64_present_spv};TRY(p->CreateShaderModule(device,&sm,NULL,&p->shader));
 VkComputePipelineCreateInfo cp={.sType=VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,.stage={.sType=VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,.stage=VK_SHADER_STAGE_COMPUTE_BIT,.module=p->shader,.pName="main"},.layout=p->layout};TRY(p->CreateComputePipelines(device,VK_NULL_HANDLE,1,&cp,NULL,&p->pipeline));
 VkSamplerCreateInfo sa={.sType=VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,.magFilter=VK_FILTER_NEAREST,.minFilter=VK_FILTER_NEAREST,.mipmapMode=VK_SAMPLER_MIPMAP_MODE_NEAREST,.addressModeU=VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,.addressModeV=VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,.addressModeW=VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE};TRY(p->CreateSampler(device,&sa,NULL,&p->sampler));
 nps64_log("[gpu-present] compositor ready; exact Q8; buffers=3 bytes=%u\n",1280*720*4*3);return p;
fail:nps64_presenter_destroy(p);return NULL;
}
void nps64_presenter_invalidate(Nps64Presenter *p){if(p)for(unsigned i=0;i<3;i++)p->previous_view[i]=VK_NULL_HANDLE;}
void *nps64_presenter_pixels(Nps64Presenter *p,unsigned i){return p&&i<3?p->pixels[i]:NULL;}
bool nps64_presenter_record(Nps64Presenter *p,VkCommandBuffer cmd,VkImageView image,VkImageLayout image_layout,unsigned index,unsigned width,unsigned height,bool wide,bool filtered){
 if(!p||index>=3||!width||!height||width>4096||height>4096)return false;
 VkDescriptorImageInfo im={p->sampler,image,image_layout};VkDescriptorBufferInfo bi={p->buffers[index],0,VK_WHOLE_SIZE};
 VkWriteDescriptorSet writes[2]={{.sType=VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,.dstSet=p->sets[index],.dstBinding=0,.descriptorCount=1,.descriptorType=VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,.pImageInfo=&im},{.sType=VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,.dstSet=p->sets[index],.dstBinding=1,.descriptorCount=1,.descriptorType=VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,.pBufferInfo=&bi}};if(p->previous_view[index]!=image||p->previous_layout[index]!=image_layout){p->UpdateDescriptorSets(p->device,2,writes,0,NULL);p->previous_view[index]=image;p->previous_layout[index]=image_layout;}
 uint32_t params[8]={width,height,wide?1280:960,720,wide?0:160,0,filtered?1:0,1280};
 p->CmdBindPipeline(cmd,VK_PIPELINE_BIND_POINT_COMPUTE,p->pipeline);p->CmdBindDescriptorSets(cmd,VK_PIPELINE_BIND_POINT_COMPUTE,p->layout,0,1,&p->sets[index],0,NULL);p->CmdPushConstants(cmd,p->layout,VK_SHADER_STAGE_COMPUTE_BIT,0,sizeof params,params);p->CmdDispatch(cmd,160,90,1);
 VkBufferMemoryBarrier b={.sType=VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,.srcAccessMask=VK_ACCESS_SHADER_WRITE_BIT,.dstAccessMask=VK_ACCESS_HOST_READ_BIT|VK_ACCESS_HOST_WRITE_BIT,.srcQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED,.dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED,.buffer=p->buffers[index],.size=VK_WHOLE_SIZE};
 p->CmdPipelineBarrier(cmd,VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,VK_PIPELINE_STAGE_HOST_BIT,0,0,NULL,1,&b,0,NULL);return true;
}
void nps64_presenter_destroy(Nps64Presenter *p){
 if(!p)return;VkDevice d=p->device;
 if(p->sampler)p->DestroySampler(d,p->sampler,NULL);if(p->pipeline)p->DestroyPipeline(d,p->pipeline,NULL);if(p->shader)p->DestroyShaderModule(d,p->shader,NULL);if(p->layout)p->DestroyPipelineLayout(d,p->layout,NULL);if(p->pool)p->DestroyDescriptorPool(d,p->pool,NULL);if(p->set_layout)p->DestroyDescriptorSetLayout(d,p->set_layout,NULL);
 for(unsigned i=0;i<3;i++){if(p->pixels[i])p->UnmapMemory(d,p->memory[i]);if(p->buffers[i])p->DestroyBuffer(d,p->buffers[i],NULL);if(p->memory[i])p->FreeMemory(d,p->memory[i],NULL);}free(p);
}
