/* Offline Vulkan compute regression: independent of ROM, UI and emulator.
 * Host only. This is NOT a PS4 executable or proof of GNM correctness. */
#include <vulkan/vulkan.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#define CHECK(call) do { VkResult r = (call); if (r != VK_SUCCESS) { \
    fprintf(stderr, "%s: %d\n", #call, r); exit(1); } } while (0)

int main(int argc, char **argv)
{
    if (argc != 2) return 2;
    FILE *f = fopen(argv[1], "rb");
    if (!f) return 2;
    fseek(f, 0, SEEK_END); long bytes = ftell(f); rewind(f);
    if (bytes <= 0 || bytes % 4) return 2;
    uint32_t *code = malloc((size_t)bytes);
    if (!code || fread(code, 1, (size_t)bytes, f) != (size_t)bytes) return 2;
    fclose(f);
    VkInstance instance;
    VkApplicationInfo app = {.sType=VK_STRUCTURE_TYPE_APPLICATION_INFO, .apiVersion=VK_API_VERSION_1_1};
    VkInstanceCreateInfo ici = {.sType=VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO, .pApplicationInfo=&app};
    CHECK(vkCreateInstance(&ici, NULL, &instance));
    uint32_t n=0; CHECK(vkEnumeratePhysicalDevices(instance, &n, NULL));
    if (!n) return 1;
    VkPhysicalDevice *devices=calloc(n,sizeof(*devices));
    CHECK(vkEnumeratePhysicalDevices(instance, &n, devices));
    VkPhysicalDevice physical=devices[0]; free(devices);
    VkPhysicalDeviceProperties props; vkGetPhysicalDeviceProperties(physical,&props);
    printf("device=%s (host; no PS4 validation)\n",props.deviceName);
    vkGetPhysicalDeviceQueueFamilyProperties(physical,&n,NULL);
    VkQueueFamilyProperties *families=calloc(n,sizeof(*families));
    vkGetPhysicalDeviceQueueFamilyProperties(physical,&n,families);
    uint32_t family=0;
    while (family<n && !(families[family].queueFlags & VK_QUEUE_COMPUTE_BIT)) family++;
    free(families); if(family==n) return 1;
    float priority=1;
    VkDeviceQueueCreateInfo qci={.sType=VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,.queueFamilyIndex=family,.queueCount=1,.pQueuePriorities=&priority};
    VkDeviceCreateInfo dci={.sType=VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,.queueCreateInfoCount=1,.pQueueCreateInfos=&qci};
    VkDevice device; CHECK(vkCreateDevice(physical,&dci,NULL,&device));
    VkQueue queue; vkGetDeviceQueue(device,family,0,&queue);
    VkBuffer buffer;
    VkBufferCreateInfo bci={.sType=VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,.size=256*sizeof(uint32_t),.usage=VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,.sharingMode=VK_SHARING_MODE_EXCLUSIVE};
    CHECK(vkCreateBuffer(device,&bci,NULL,&buffer));
    VkMemoryRequirements req; vkGetBufferMemoryRequirements(device,buffer,&req);
    VkPhysicalDeviceMemoryProperties memory; vkGetPhysicalDeviceMemoryProperties(physical,&memory);
    uint32_t mt=0;
    for(;mt<memory.memoryTypeCount;mt++) if((req.memoryTypeBits&(1u<<mt)) &&
        (memory.memoryTypes[mt].propertyFlags&(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)) ==
        (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)) break;
    if(mt==memory.memoryTypeCount) return 1;
    VkMemoryAllocateInfo mai={.sType=VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,.allocationSize=req.size,.memoryTypeIndex=mt};
    VkDeviceMemory mem; CHECK(vkAllocateMemory(device,&mai,NULL,&mem));
    CHECK(vkBindBufferMemory(device,buffer,mem,0));
    uint32_t *mapped; CHECK(vkMapMemory(device,mem,0,VK_WHOLE_SIZE,0,(void**)&mapped));
    VkDescriptorSetLayoutBinding binding={.binding=0,.descriptorType=VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,.descriptorCount=1,.stageFlags=VK_SHADER_STAGE_COMPUTE_BIT};
    VkDescriptorSetLayoutCreateInfo slci={.sType=VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,.bindingCount=1,.pBindings=&binding};
    VkDescriptorSetLayout setlayout; CHECK(vkCreateDescriptorSetLayout(device,&slci,NULL,&setlayout));
    VkPipelineLayoutCreateInfo plci={.sType=VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,.setLayoutCount=1,.pSetLayouts=&setlayout};
    VkPipelineLayout layout; CHECK(vkCreatePipelineLayout(device,&plci,NULL,&layout));
    VkDescriptorPoolSize size={VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,1};
    VkDescriptorPoolCreateInfo dpci={.sType=VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,.maxSets=1,.poolSizeCount=1,.pPoolSizes=&size};
    VkDescriptorPool pool; CHECK(vkCreateDescriptorPool(device,&dpci,NULL,&pool));
    VkDescriptorSetAllocateInfo dsai={.sType=VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,.descriptorPool=pool,.descriptorSetCount=1,.pSetLayouts=&setlayout};
    VkDescriptorSet set; CHECK(vkAllocateDescriptorSets(device,&dsai,&set));
    VkDescriptorBufferInfo dbi={buffer,0,bci.size};
    VkWriteDescriptorSet write={.sType=VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,.dstSet=set,.descriptorCount=1,.descriptorType=VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,.pBufferInfo=&dbi};
    vkUpdateDescriptorSets(device,1,&write,0,NULL);
    VkShaderModuleCreateInfo smci={.sType=VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,.codeSize=(size_t)bytes,.pCode=code};
    VkShaderModule shader; CHECK(vkCreateShaderModule(device,&smci,NULL,&shader)); free(code);
    VkCommandPoolCreateInfo cpci={.sType=VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,.flags=VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,.queueFamilyIndex=family};
    VkCommandPool commands; CHECK(vkCreateCommandPool(device,&cpci,NULL,&commands));
    VkCommandBufferAllocateInfo cbai={.sType=VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,.commandPool=commands,.level=VK_COMMAND_BUFFER_LEVEL_PRIMARY,.commandBufferCount=1};
    VkCommandBuffer cmd; CHECK(vkAllocateCommandBuffers(device,&cbai,&cmd));
    VkFenceCreateInfo fci={.sType=VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
    VkFence fence; CHECK(vkCreateFence(device,&fci,NULL,&fence));
    VkSpecializationMapEntry entries[]={{0,0,4},{1,4,4}};
    for(uint32_t test=0;test<3;test++) {
        uint32_t constants[]={test==1?32u:64u,test==0?0u:1u};
        VkSpecializationInfo spec={2,entries,sizeof(constants),constants};
        VkComputePipelineCreateInfo pci={.sType=VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
            .stage={.sType=VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,.stage=VK_SHADER_STAGE_COMPUTE_BIT,.module=shader,.pName="main",.pSpecializationInfo=&spec},.layout=layout};
        VkPipeline pipeline; CHECK(vkCreateComputePipelines(device,VK_NULL_HANDLE,1,&pci,NULL,&pipeline));
        memset(mapped,0xcd,(size_t)bci.size);
        CHECK(vkResetCommandBuffer(cmd,0));
        VkCommandBufferBeginInfo begin={.sType=VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
        CHECK(vkBeginCommandBuffer(cmd,&begin));
        vkCmdBindPipeline(cmd,VK_PIPELINE_BIND_POINT_COMPUTE,pipeline);
        vkCmdBindDescriptorSets(cmd,VK_PIPELINE_BIND_POINT_COMPUTE,layout,0,1,&set,0,NULL);
        // Repeat dispatch with compute dependency, then a host visibility barrier.
        for(unsigned repeat=0;repeat<2;repeat++) {
            vkCmdDispatch(cmd,4,1,1);
            VkMemoryBarrier barrier={.sType=VK_STRUCTURE_TYPE_MEMORY_BARRIER,.srcAccessMask=VK_ACCESS_SHADER_WRITE_BIT,
                .dstAccessMask=repeat?VK_ACCESS_HOST_READ_BIT:VK_ACCESS_SHADER_WRITE_BIT};
            vkCmdPipelineBarrier(cmd,VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                repeat?VK_PIPELINE_STAGE_HOST_BIT:VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,0,1,&barrier,0,NULL,0,NULL);
        }
        CHECK(vkEndCommandBuffer(cmd));
        VkSubmitInfo submit={.sType=VK_STRUCTURE_TYPE_SUBMIT_INFO,.commandBufferCount=1,.pCommandBuffers=&cmd};
        CHECK(vkQueueSubmit(queue,1,&submit,fence));
        // A host timeout detects failure; it cannot reset a hung physical GPU.
        CHECK(vkWaitForFences(device,1,&fence,VK_TRUE,5000000000ull));
        for(uint32_t i=0;i<256;i++) {
            uint32_t expected=i<constants[0]*4?(test==0?(i^0x64a5u):32u):0xcdcdcdcdu;
            if(mapped[i]!=expected) {fprintf(stderr,"probe=%u index=%u actual=%x expected=%x\n",test,i,mapped[i],expected);return 1;}
        }
        printf("PASS %s local=%u: two dispatches, dependency, exact output and guard\n",test==0?"storage":"LDS/atomic/barrier/bitmask",constants[0]);
        CHECK(vkResetFences(device,1,&fence));
        vkDestroyPipeline(device,pipeline,NULL);
    }
    vkDestroyFence(device,fence,NULL); vkDestroyCommandPool(device,commands,NULL);
    vkDestroyShaderModule(device,shader,NULL); vkDestroyDescriptorPool(device,pool,NULL);
    vkDestroyPipelineLayout(device,layout,NULL); vkDestroyDescriptorSetLayout(device,setlayout,NULL);
    vkUnmapMemory(device,mem); vkDestroyBuffer(device,buffer,NULL); vkFreeMemory(device,mem,NULL);
    vkDestroyDevice(device,NULL); vkDestroyInstance(instance,NULL);
    return 0;
}
