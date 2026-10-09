"""Replay the captured first raster dispatch with upstream SPIR-V on host Vulkan.

Snapshots were taken after the PS4 hang, so this checks termination and bounds
with those inputs, not exact pre-dispatch state or Liverpool machine code.
"""
from pathlib import Path
import os, subprocess
root=Path('/work'); work=root/'build/capture-replay';work.mkdir(parents=True,exist_ok=True)
s=(root/'tests/gpu_compute_probe.c').read_text()
def replace(a,b):
 global s
 assert s.count(a)==1,a
 s=s.replace(a,b)
replace('.apiVersion=VK_API_VERSION_1_1','.apiVersion=VK_API_VERSION_1_2')
replace('VkDeviceCreateInfo dci=', '''VkPhysicalDeviceVulkan11Features f11={.sType=VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES,.storageBuffer16BitAccess=VK_TRUE};
    VkPhysicalDeviceVulkan12Features f12={.sType=VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES,.pNext=&f11,.storageBuffer8BitAccess=VK_TRUE};
    VkDeviceCreateInfo dci=''')
replace('.queueCreateInfoCount=1,.pQueueCreateInfos=&qci}', '.pNext=&f12,.queueCreateInfoCount=1,.pQueueCreateInfos=&qci}')
replace('.size=256*sizeof(uint32_t)','.size=32*1024*1024')
replace('.usage=VK_BUFFER_USAGE_STORAGE_BUFFER_BIT', '.usage=VK_BUFFER_USAGE_STORAGE_BUFFER_BIT|VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT|VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT')
a=s.index('    VkDescriptorSetLayoutBinding binding=');b=s.index('    VkShaderModuleCreateInfo',a)
addresses=['43fbf40000','43f7f20000','43f8320000',
 # RADV packs only active bindings. Scissor binding 3 is unused by the
 # rasterizer; supply an unused placeholder when rebuilding the GLSL layout.
 '43efe20000','43efe22000','43efe2a000','43efe2e000','43efe2f000','43efe2f800',
 '43efe30000','43efe32000','43f8440000','43efe33000','43ebdc0000','43f8640000','43f8321000','43e3bc0000']
offsets=[];sizes=[];cursor=0
for address in addresses:
 size=(root/f'gpu-hang-data-{address}.bin').stat().st_size
 offsets.append(cursor);sizes.append(size);cursor=(cursor+size+255)&~255
assert cursor<32*1024*1024
array=lambda items:', '.join(map(str,items))
setup='''    const VkDeviceSize offsets[]={OFFSETS};
    const VkDeviceSize sizes[]={SIZES};
    const char *files[]={FILES};
    memset(mapped,0,(size_t)bci.size);
    for(unsigned i=0;i<17;i++) {
        FILE *input=fopen(files[i],"rb"); if(!input)return 2;
        if(fread((char*)mapped+offsets[i],1,sizes[i],input)!=sizes[i])return 2;
        fclose(input);
    }
    VkDescriptorSetLayout layouts[3];
    VkDescriptorSetLayoutBinding bindings[13];
    unsigned counts[]={3,13,1};
    for(unsigned set=0;set<3;set++) {
        for(unsigned i=0;i<counts[set];i++) bindings[i]=(VkDescriptorSetLayoutBinding){
            .binding=i,.descriptorType=set==2?VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
                (set==1&&i==10?VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:VK_DESCRIPTOR_TYPE_STORAGE_BUFFER),
            .descriptorCount=1,.stageFlags=VK_SHADER_STAGE_COMPUTE_BIT};
        VkDescriptorSetLayoutCreateInfo ci={.sType=VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
            .bindingCount=counts[set],.pBindings=bindings};
        CHECK(vkCreateDescriptorSetLayout(device,&ci,NULL,&layouts[set]));
    }
    VkPushConstantRange push={VK_SHADER_STAGE_COMPUTE_BIT,0,20};
    VkPipelineLayoutCreateInfo plci={.sType=VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount=3,.pSetLayouts=layouts,.pushConstantRangeCount=1,.pPushConstantRanges=&push};
    VkPipelineLayout layout; CHECK(vkCreatePipelineLayout(device,&plci,NULL,&layout));
    VkDescriptorPoolSize pools[]={{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,15},
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,1},{VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER,1}};
    VkDescriptorPoolCreateInfo dpci={.sType=VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .maxSets=3,.poolSizeCount=3,.pPoolSizes=pools};
    VkDescriptorPool pool;CHECK(vkCreateDescriptorPool(device,&dpci,NULL,&pool));
    VkDescriptorSetAllocateInfo dsai={.sType=VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool=pool,.descriptorSetCount=3,.pSetLayouts=layouts};
    VkDescriptorSet sets[3]; CHECK(vkAllocateDescriptorSets(device,&dsai,sets));
    VkBufferViewCreateInfo viewci={.sType=VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO,
        .buffer=buffer,.format=VK_FORMAT_R8_UINT,.offset=offsets[13],.range=sizes[13]};
    VkBufferView view;CHECK(vkCreateBufferView(device,&viewci,NULL,&view));
    unsigned n=0;
    for(unsigned set=0;set<3;set++) for(unsigned binding=0;binding<counts[set];binding++,n++) {
        VkDescriptorBufferInfo info={buffer,offsets[n],sizes[n]};
        VkWriteDescriptorSet write={.sType=VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet=sets[set],.dstBinding=binding,.descriptorCount=1,
            .descriptorType=set==2?VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
                (set==1&&binding==10?VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:VK_DESCRIPTOR_TYPE_STORAGE_BUFFER),
            .pBufferInfo=&info,.pTexelBufferView=&view};
        vkUpdateDescriptorSets(device,1,&write,0,NULL);
    }
'''.replace('OFFSETS',array(offsets)).replace('SIZES',array(sizes)).replace('FILES',', '.join('"/work/gpu-hang-data-'+v+'.bin"' for v in addresses))
# n is already the physical-device count in the probe.
setup=setup.replace('unsigned n=0;','unsigned descriptor_index=0;').replace('binding++,n++','binding++,descriptor_index++').replace('offsets[n],sizes[n]','offsets[descriptor_index],sizes[descriptor_index]')
s=s[:a]+setup+s[b:]
a=s.index('    VkSpecializationMapEntry entries[]=');b=s.index('    vkDestroyFence(',a)
s=s[:a]+'''    VkSpecializationMapEntry entries[8];for(unsigned i=0;i<8;i++)entries[i]=(VkSpecializationMapEntry){i,i*4,4};
    uint32_t constants[]={8*1024*1024,2,0,8,8,256,1024,1};
    VkSpecializationInfo spec={8,entries,sizeof(constants),constants};
    VkComputePipelineCreateInfo pci={.sType=VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
        .stage={.sType=VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,.stage=VK_SHADER_STAGE_COMPUTE_BIT,.module=shader,.pName="main",.pSpecializationInfo=&spec},.layout=layout};
    VkPipeline pipeline;CHECK(vkCreateComputePipelines(device,VK_NULL_HANDLE,1,&pci,NULL,&pipeline));
    VkCommandBufferBeginInfo begin={.sType=VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};CHECK(vkBeginCommandBuffer(cmd,&begin));
    vkCmdBindPipeline(cmd,VK_PIPELINE_BIND_POINT_COMPUTE,pipeline);
    vkCmdBindDescriptorSets(cmd,VK_PIPELINE_BIND_POINT_COMPUTE,layout,0,3,sets,0,NULL);
    uint32_t registers[]={0x18fbc0,0,320,239,1};
    vkCmdPushConstants(cmd,layout,VK_SHADER_STAGE_COMPUTE_BIT,0,sizeof(registers),registers);
    vkCmdDispatch(cmd,40,30,1);
    VkMemoryBarrier barrier={.sType=VK_STRUCTURE_TYPE_MEMORY_BARRIER,.srcAccessMask=VK_ACCESS_SHADER_WRITE_BIT,.dstAccessMask=VK_ACCESS_HOST_READ_BIT};
    vkCmdPipelineBarrier(cmd,VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,VK_PIPELINE_STAGE_HOST_BIT,0,1,&barrier,0,NULL,0,NULL);
    CHECK(vkEndCommandBuffer(cmd));VkSubmitInfo submit={.sType=VK_STRUCTURE_TYPE_SUBMIT_INFO,.commandBufferCount=1,.pCommandBuffers=&cmd};
    CHECK(vkQueueSubmit(queue,1,&submit,fence));CHECK(vkWaitForFences(device,1,&fence,VK_TRUE,10000000000ull));
    FILE *output=fopen("/work/build/capture-replay/result-vram.bin","wb");if(!output)return 2;
    fwrite(mapped,1,sizes[0],output);fclose(output);
    output=fopen("/work/build/capture-replay/result-hidden.bin","wb");if(!output)return 2;
    fwrite((char*)mapped+offsets[1],1,sizes[1],output);fclose(output);
    printf("PASS captured raster dispatch completed (host SPIR-V; not PS4 ISA)\\n");
    vkDestroyPipeline(device,pipeline,NULL);vkDestroyBufferView(device,view,NULL);
''' +s[b:]
s=s.replace('vkDestroyDescriptorSetLayout(device,setlayout,NULL);','for(unsigned i=0;i<3;i++)vkDestroyDescriptorSetLayout(device,layouts[i],NULL);')
(work/'replay.c').write_text(s)
includes=root/'vendor/parallel-n64/mupen64plus-video-paraLLEl/parallel-rdp/parallel-rdp/shaders'
spv=work/'ubershader.spv'
glsl=work/'ubershader.comp'
glsl.write_text((includes/'ubershader.comp').read_text().replace('#version 450','#version 450\n#extension GL_GOOGLE_include_directive : require',1))
subprocess.run([str(root/'build/gpu/glslang-host/StandAlone/glslang'),'-V','--target-env','vulkan1.1','-I'+str(includes),'-DSMALL_TYPES=0','-DSUBGROUP=0','-DDEBUG_ENABLE=0',str(glsl),'-o',str(spv)],check=True)
subprocess.run(['clang-21',str(work/'replay.c'),'-lvulkan','-o',str(work/'replay')],check=True)
env=dict(os.environ,VK_ICD_FILENAMES='/usr/share/vulkan/icd.d/lvp_icd.x86_64.json',VK_INSTANCE_LAYERS='VK_LAYER_KHRONOS_validation')
subprocess.run([str(work/'replay'),str(spv)],check=True,env=env,timeout=40)
reference=[(work/name).read_bytes() for name in ['result-vram.bin','result-hidden.bin']]
subprocess.run([str(work/'replay'),str(root/'build/gpu-shaders/color_raster.spv')],check=True,env=env,timeout=40)
for name, expected in zip(['result-vram.bin','result-hidden.bin'],reference):
 assert (work/name).read_bytes()==expected, 'Color-only mismatch: '+name
print('PASS color-only raster matches original: all 16 MiB VRAM and 4 MiB hidden VRAM')
