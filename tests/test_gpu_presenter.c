#include "gpu_presenter.h"
#include "gpu_readback_memory.h"
#include "bilinear_scaler.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define OK(x) assert((x)==VK_SUCCESS)
static unsigned memory_type(VkPhysicalDevice gpu,unsigned bits,VkMemoryPropertyFlags flags){VkPhysicalDeviceMemoryProperties p;vkGetPhysicalDeviceMemoryProperties(gpu,&p);for(unsigned i=0;i<p.memoryTypeCount;i++)if((bits&(1u<<i))&&(p.memoryTypes[i].propertyFlags&flags)==flags)return i;assert(0);return 0;}
static Nps64BilinearScaler scaler;
int main(void){
 VkInstance instance;VkApplicationInfo app={.sType=VK_STRUCTURE_TYPE_APPLICATION_INFO,.apiVersion=VK_API_VERSION_1_1};VkInstanceCreateInfo ic={.sType=VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,.pApplicationInfo=&app};OK(vkCreateInstance(&ic,NULL,&instance));
 uint32_t count=1;VkPhysicalDevice gpu;OK(vkEnumeratePhysicalDevices(instance,&count,&gpu));uint32_t n=0;vkGetPhysicalDeviceQueueFamilyProperties(gpu,&n,NULL);VkQueueFamilyProperties *props=calloc(n,sizeof(*props));vkGetPhysicalDeviceQueueFamilyProperties(gpu,&n,props);unsigned family=0;while(family<n&&!(props[family].queueFlags&VK_QUEUE_COMPUTE_BIT))family++;assert(family<n);free(props);
 float priority=1;VkDeviceQueueCreateInfo qi={.sType=VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,.queueFamilyIndex=family,.queueCount=1,.pQueuePriorities=&priority};VkDeviceCreateInfo di={.sType=VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,.queueCreateInfoCount=1,.pQueueCreateInfos=&qi};VkDevice device;OK(vkCreateDevice(gpu,&di,NULL,&device));VkQueue queue;vkGetDeviceQueue(device,family,0,&queue);
 Nps64Presenter *p=nps64_presenter_create(gpu,device,vkGetInstanceProcAddr,instance,vkGetDeviceProcAddr);assert(p);
 VkCommandPool pool;VkCommandPoolCreateInfo pi={.sType=VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,.queueFamilyIndex=family};OK(vkCreateCommandPool(device,&pi,NULL,&pool));VkCommandBuffer cmd;VkCommandBufferAllocateInfo ca={.sType=VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,.commandPool=pool,.level=VK_COMMAND_BUFFER_LEVEL_PRIMARY,.commandBufferCount=1};OK(vkAllocateCommandBuffers(device,&ca,&cmd));
 VkFence fence;VkFenceCreateInfo fi={.sType=VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};OK(vkCreateFence(device,&fi,NULL,&fence));
 const unsigned cases[][2]={{1,1},{3,5},{320,240},{640,480},{1280,480},{1280,960},{640,237},{320,237},{854,480}};
 uint32_t *expected=malloc(1280*720*4);assert(expected);unsigned runs=0;
 for(unsigned c=0;c<sizeof cases/sizeof *cases;c++){
  unsigned w=cases[c][0],h=cases[c][1];size_t bytes=(size_t)w*h*4;
  VkBuffer staging;VkBufferCreateInfo bc={.sType=VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,.size=bytes,.usage=VK_BUFFER_USAGE_TRANSFER_SRC_BIT,.sharingMode=VK_SHARING_MODE_EXCLUSIVE};OK(vkCreateBuffer(device,&bc,NULL,&staging));VkMemoryRequirements req;vkGetBufferMemoryRequirements(device,staging,&req);
  VkDeviceMemory sm;VkMemoryAllocateInfo ma={.sType=VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,.allocationSize=req.size,.memoryTypeIndex=memory_type(gpu,req.memoryTypeBits,VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)};OK(vkAllocateMemory(device,&ma,NULL,&sm));OK(vkBindBufferMemory(device,staging,sm,0));uint32_t *input;OK(vkMapMemory(device,sm,0,VK_WHOLE_SIZE,0,(void**)&input));
  uint32_t seed=17;for(unsigned i=0;i<w*h;i++){seed=seed*1664525+1013904223;input[i]=seed;}
  VkImage image;VkImageCreateInfo ii={.sType=VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,.imageType=VK_IMAGE_TYPE_2D,.format=VK_FORMAT_R8G8B8A8_UNORM,.extent={w,h,1},.mipLevels=1,.arrayLayers=1,.samples=VK_SAMPLE_COUNT_1_BIT,.tiling=VK_IMAGE_TILING_OPTIMAL,.usage=VK_IMAGE_USAGE_SAMPLED_BIT|VK_IMAGE_USAGE_TRANSFER_DST_BIT,.sharingMode=VK_SHARING_MODE_EXCLUSIVE};OK(vkCreateImage(device,&ii,NULL,&image));vkGetImageMemoryRequirements(device,image,&req);ma.allocationSize=req.size;ma.memoryTypeIndex=memory_type(gpu,req.memoryTypeBits,0);VkDeviceMemory im;OK(vkAllocateMemory(device,&ma,NULL,&im));OK(vkBindImageMemory(device,image,im,0));
  VkImageView view;VkImageViewCreateInfo vi={.sType=VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,.image=image,.viewType=VK_IMAGE_VIEW_TYPE_2D,.format=VK_FORMAT_R8G8B8A8_UNORM,.subresourceRange={VK_IMAGE_ASPECT_COLOR_BIT,0,1,0,1}};OK(vkCreateImageView(device,&vi,NULL,&view));
  for(unsigned mode=0;mode<4;mode++){
   bool wide=(mode&1)!=0,filtered=(mode&2)!=0;unsigned dw=wide?1280:960,left=wide?0:160,index=runs%3;
   /* Simulate CPU HUD/pause contamination from the preceding use. */
   memset(nps64_presenter_pixels(p,index),0x5c,1280*720*4);
   OK(vkResetCommandPool(device,pool,0));VkCommandBufferBeginInfo begin={.sType=VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};OK(vkBeginCommandBuffer(cmd,&begin));
   VkImageMemoryBarrier barrier={.sType=VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,.srcAccessMask=mode?VK_ACCESS_SHADER_READ_BIT:0,.dstAccessMask=VK_ACCESS_TRANSFER_WRITE_BIT,.oldLayout=mode?VK_IMAGE_LAYOUT_GENERAL:VK_IMAGE_LAYOUT_UNDEFINED,.newLayout=VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,.srcQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED,.dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED,.image=image,.subresourceRange={VK_IMAGE_ASPECT_COLOR_BIT,0,1,0,1}};vkCmdPipelineBarrier(cmd,mode?VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT:VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT,0,0,NULL,0,NULL,1,&barrier);
   VkBufferImageCopy copy={.imageSubresource={VK_IMAGE_ASPECT_COLOR_BIT,0,0,1},.imageExtent={w,h,1}};vkCmdCopyBufferToImage(cmd,staging,image,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,1,&copy);
   barrier.srcAccessMask=VK_ACCESS_TRANSFER_WRITE_BIT;barrier.dstAccessMask=VK_ACCESS_SHADER_READ_BIT;barrier.oldLayout=VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;barrier.newLayout=VK_IMAGE_LAYOUT_GENERAL;vkCmdPipelineBarrier(cmd,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,0,0,NULL,0,NULL,1,&barrier);
   assert(nps64_presenter_record(p,cmd,view,VK_IMAGE_LAYOUT_GENERAL,index,w,h,wide,filtered));OK(vkEndCommandBuffer(cmd));OK(vkResetFences(device,1,&fence));VkSubmitInfo si={.sType=VK_STRUCTURE_TYPE_SUBMIT_INFO,.commandBufferCount=1,.pCommandBuffers=&cmd};OK(vkQueueSubmit(queue,1,&si,fence));OK(vkWaitForFences(device,1,&fence,VK_TRUE,10000000000ULL));
   for(unsigned i=0;i<1280*720;i++)expected[i]=0xff000000;
   if(filtered)assert(nps64_scale_bilinear32(&scaler,expected+left,1280,input,w*4,w,h,dw,720,1));
   else for(unsigned y=0;y<720;y++)for(unsigned x=0;x<dw;x++){uint32_t v=input[(size_t)(y*h/720)*w+x*w/dw];expected[(size_t)y*1280+left+x]=0xff000000|((v&255)<<16)|(v&0xff00)|((v>>16)&255);}
   const uint32_t *actual=nps64_presenter_pixels(p,index);for(unsigned i=0;i<1280*720;i++)if(actual[i]!=expected[i]){fprintf(stderr,"mismatch %ux%u mode=%u pixel=%u actual=%08x expected=%08x\n",w,h,mode,i,actual[i],expected[i]);abort();}runs++;
  }
  nps64_presenter_invalidate(p);
  vkDestroyImageView(device,view,NULL);vkDestroyImage(device,image,NULL);vkFreeMemory(device,im,NULL);vkUnmapMemory(device,sm);vkDestroyBuffer(device,staging,NULL);vkFreeMemory(device,sm,NULL);
 }
 free(expected);OK(vkDeviceWaitIdle(device));nps64_presenter_destroy(p);vkDestroyFence(device,fence,NULL);vkDestroyCommandPool(device,pool,NULL);vkDestroyDevice(device,NULL);vkDestroyInstance(instance,NULL);printf("PASS GPU compositor: %u full frames pixel-exact vs CPU; native/2x, odd sizes, borders, filter/aspect changes and HUD overwrite\n",runs);return 0;
}
