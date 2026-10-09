"""Run original and direct tile shaders on identical multi-group inputs in Vulkan.

Reuse the standalone compute probe's device/buffer lifecycle; this is a host
oracle test, not a simulation of Liverpool or proof of PS4 stability.
"""
from pathlib import Path
import os, subprocess
root=Path('/work'); work=root/'build/gpu-tile-equivalence'; work.mkdir(parents=True,exist_ok=True)
source=(root/'tests/gpu_compute_probe.c').read_text()
def replace(old,new):
 global source
 assert source.count(old)==1, old
 source=source.replace(old,new)
replace('if (argc != 2) return 2;', 'if (argc != 5) return 2;\n    unsigned direct=atoi(argv[2]), count=atoi(argv[3]);\n    if(count<1 || count>256) return 2;')
replace('.apiVersion=VK_API_VERSION_1_1', '.apiVersion=VK_API_VERSION_1_2')
replace('VkDeviceCreateInfo dci=', '''VkPhysicalDeviceVulkan11Features f11={.sType=VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES,.storageBuffer16BitAccess=VK_TRUE};
    VkPhysicalDeviceVulkan12Features f12={.sType=VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES,.pNext=&f11,.storageBuffer8BitAccess=VK_TRUE};
    VkDeviceCreateInfo dci=''')
replace('.queueCreateInfoCount=1,.pQueueCreateInfos=&qci}', '.pNext=&f12,.queueCreateInfoCount=1,.pQueueCreateInfos=&qci}')
replace('.size=256*sizeof(uint32_t)', '.size=524288')
replace('VkDescriptorSetLayoutBinding binding={.binding=0,.descriptorType=VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,.descriptorCount=1,.stageFlags=VK_SHADER_STAGE_COMPUTE_BIT};', '''VkDescriptorSetLayoutBinding binding[5];
    for(unsigned i=0;i<5;i++) binding[i]=(VkDescriptorSetLayoutBinding){.binding=i,.descriptorType=VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,.descriptorCount=1,.stageFlags=VK_SHADER_STAGE_COMPUTE_BIT};''')
replace('.bindingCount=1,.pBindings=&binding', '.bindingCount=5,.pBindings=binding')
replace('VkPipelineLayoutCreateInfo plci=', 'VkPushConstantRange push={VK_SHADER_STAGE_COMPUTE_BIT,0,12};\n    VkPipelineLayoutCreateInfo plci=')
replace('.setLayoutCount=1,.pSetLayouts=&setlayout}', '.setLayoutCount=1,.pSetLayouts=&setlayout,.pushConstantRangeCount=1,.pPushConstantRanges=&push}')
replace('VkDescriptorPoolSize size={VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,1}', 'VkDescriptorPoolSize size={VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,5}')
a=source.index('    VkDescriptorBufferInfo dbi=');b=source.index('    VkShaderModuleCreateInfo',a)
source=source[:a]+'''    VkDescriptorBufferInfo dbi[5]; VkWriteDescriptorSet write[5];
    const VkDeviceSize offsets[]={0,65536,131072,196608,393216};
    const VkDeviceSize sizes[]={8192,4096,4096,131072,16384};
    for(unsigned i=0;i<5;i++) {
        dbi[i]=(VkDescriptorBufferInfo){buffer,offsets[i],sizes[i]};
        write[i]=(VkWriteDescriptorSet){.sType=VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,.dstSet=set,.dstBinding=i,.descriptorCount=1,.descriptorType=VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,.pBufferInfo=&dbi[i]};
    }
    vkUpdateDescriptorSets(device,5,write,0,NULL);
''' +source[b:]
a=source.index('    VkSpecializationMapEntry entries[]=');b=source.index('    vkDestroyFence(',a)
source=source[:a]+'''    VkSpecializationMapEntry entries[7];
    for(unsigned i=0;i<7;i++) entries[i]=(VkSpecializationMapEntry){i,i*4,4};
    uint32_t constants[]={direct?64:32,8,8,256,1024,0x8000,1};
    VkSpecializationInfo spec={7,entries,sizeof(constants),constants};
    VkComputePipelineCreateInfo pci={.sType=VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
        .stage={.sType=VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,.stage=VK_SHADER_STAGE_COMPUTE_BIT,.module=shader,.pName="main",.pSpecializationInfo=&spec},.layout=layout};
    VkPipeline pipeline; CHECK(vkCreateComputePipelines(device,VK_NULL_HANDLE,1,&pci,NULL,&pipeline));
    memset(mapped,0xcd,(size_t)bci.size);
    struct Triangle {int32_t xh,xm,xl;int16_t yh,ym;int32_t dxhdy,dxmdy,dxldy;int16_t yl;uint8_t flags,tile;};
    _Static_assert(sizeof(struct Triangle)==32,"Triangle ABI");
    struct Triangle *tri=(void*)mapped;
    int32_t *scissor=(void*)((char*)mapped+offsets[1]);
    for(unsigned i=0;i<count;i++) {
        int left=(int)(i*13%260),right=left+20+(int)(i%40);
        tri[i]=(struct Triangle){.xh=(i&1?left:right)*65536,.xm=(i&1?right:left)*65536,.xl=(i&1?right:left)*65536,
            .yh=(int16_t)((i%7)*4),.ym=400,.yl=(int16_t)(900+i%50),.flags=(uint8_t)((i&1)|128)};
        scissor[4*i]=4*(int)(i%9);scissor[4*i+1]=4*(int)(i%5);
        scissor[4*i+2]=4*(321-(int)(i%13));scissor[4*i+3]=4*(241-(int)(i%11));
    }
    // Original coarse masks use atomic operations; initialize as the renderer does.
    memset((char*)mapped+offsets[4],0,(size_t)sizes[4]);
    VkCommandBufferBeginInfo begin={.sType=VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    CHECK(vkBeginCommandBuffer(cmd,&begin));
    vkCmdBindPipeline(cmd,VK_PIPELINE_BIND_POINT_COMPUTE,pipeline);
    vkCmdBindDescriptorSets(cmd,VK_PIPELINE_BIND_POINT_COMPUTE,layout,0,1,&set,0,NULL);
    uint32_t registers[]={321,241,count};
    vkCmdPushConstants(cmd,layout,VK_SHADER_STAGE_COMPUTE_BIT,0,12,registers);
    vkCmdDispatch(cmd,direct?1:(count+31)/32,6,direct?4:8);
    VkMemoryBarrier barrier={.sType=VK_STRUCTURE_TYPE_MEMORY_BARRIER,.srcAccessMask=VK_ACCESS_SHADER_WRITE_BIT,.dstAccessMask=VK_ACCESS_HOST_READ_BIT};
    vkCmdPipelineBarrier(cmd,VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,VK_PIPELINE_STAGE_HOST_BIT,0,1,&barrier,0,NULL,0,NULL);
    CHECK(vkEndCommandBuffer(cmd));
    VkSubmitInfo submit={.sType=VK_STRUCTURE_TYPE_SUBMIT_INFO,.commandBufferCount=1,.pCommandBuffers=&cmd};
    CHECK(vkQueueSubmit(queue,1,&submit,fence));
    CHECK(vkWaitForFences(device,1,&fence,VK_TRUE,5000000000ull));
    f=fopen(argv[4],"wb"); if(!f) return 2;
    uint32_t *masks=(void*)((char*)mapped+offsets[3]), *coarse=(void*)((char*)mapped+offsets[4]);
    unsigned lit=0;
    for(unsigned y=0;y<31;y++) for(unsigned x=0;x<41;x++) {
        unsigned tile=y*128+x;
        for(unsigned g=0;g<(count+31)/32;g++) {
            uint32_t value=masks[tile*8+g];lit+=value!=0;
            fwrite(&value,4,1,f);
        }
        fwrite(&coarse[tile],4,1,f);
    }
    fclose(f); if(!lit) return 1;
    // Untouched bytes immediately after output buffers catch overruns.
    for(unsigned i=0;i<16;i++) if(((unsigned char*)mapped)[offsets[4]+sizes[4]+i]!=0xcd) return 1;
    printf("PASS binning direct=%u primitives=%u nonzero_masks=%u\\n",direct,count,lit);
    vkDestroyPipeline(device,pipeline,NULL);
''' +source[b:]
harness=work/'harness.c';harness.write_text(source)
subprocess.run(['clang-21','-std=c11','-O2','-Wall','-Wextra','-Werror',str(harness),'-lvulkan','-o',str(work/'probe')],check=True)
includes=root/'vendor/parallel-n64/mupen64plus-video-paraLLEl/parallel-rdp/parallel-rdp/shaders'
original=(includes/'tile_binning_combined.comp').read_text().replace('#version 450','#version 450\n#extension GL_GOOGLE_include_directive : require\n#define SUBGROUP 0\n#define UBERSHADER 1\n#define SMALL_TYPES 0\n#define DEBUG_ENABLE 0',1)
(work/'original.comp').write_text(original)
subprocess.run([str(root/'build/gpu/glslang-host/StandAlone/glslang'),'-V','--target-env','vulkan1.1','-I'+str(includes),str(work/'original.comp'),'-o',str(work/'original.spv')],check=True)
env=dict(os.environ,VK_ICD_FILENAMES='/usr/share/vulkan/icd.d/lvp_icd.x86_64.json',VK_INSTANCE_LAYERS='VK_LAYER_KHRONOS_validation')
with (work/'results.log').open('w') as log:
 for count in [1,31,32,33,64,255,256]:
  for direct in [0,1]:
   spv=work/'original.spv' if not direct else root/'build/gpu-shaders/tile_binning_direct.spv'
   subprocess.run(['timeout','30',str(work/'probe'),str(spv),str(direct),str(count),str(work/f'{count}-{direct}.bin')],env=env,stdout=log,stderr=log,check=True)
  assert (work/f'{count}-0.bin').read_bytes()==(work/f'{count}-1.bin').read_bytes(), f'GPU masks differ at count {count}'
s=(work/'results.log').read_text();assert 'VUID-' not in s and 'Validation Error' not in s,s
print(s)
print('PASS: original/direct GPU masks and coarse masks identical for 1,31,32,33,64,255,256 primitives and partial edge tiles')
