"""Embed vertex/fragment shaders for the first HLE backend hardware gate."""
from pathlib import Path
import struct, subprocess, re
root=Path('/work');out=root/'build/hle-probe';out.mkdir(parents=True,exist_ok=True)
header='#pragma once\n#include <stdint.h>\n'
for stage in ('vert','frag'):
    spv=out/('probe.'+stage+'.spv')
    subprocess.run([str(root/'build/gpu/glslang-host/StandAlone/glslang'),'-V','--target-env','vulkan1.1',
                    str(root/('src/gpu_shaders/hle_probe.'+stage)),'-o',str(spv)],check=True)
    words=struct.unpack('<%dI'%(spv.stat().st_size//4),spv.read_bytes())
    header+='static const uint32_t hle_'+stage+'[]={\n'
    for i in range(0,len(words),8):header+=','.join('0x%08xu'%w for w in words[i:i+8])+',\n'
    header+='};\n'
(root/'src/gpu_shaders/hle_probe_spv.h').write_text(header)
print('PASS embedded graphics shaders (no compute entry point)')
source=(root/'src/gpu_hle_probe.c').read_text()
functions=sorted(set(re.findall(r'\b(vk[A-Z]\w+)\s*\(',source)))
instance_names={'vkDestroyInstance','vkEnumeratePhysicalDevices','vkGetPhysicalDeviceProperties',
                'vkGetPhysicalDeviceQueueFamilyProperties','vkGetPhysicalDeviceMemoryProperties','vkCreateDevice'}
global_names={'vkCreateInstance'}
dispatch='#pragma once\nextern PFN_vkVoidFunction vk_icdGetInstanceProcAddr(VkInstance,const char*);\n'
dispatch+='static PFN_vkGetDeviceProcAddr hle_gdpa;\n'
for name in functions:
    dispatch+=f'static PFN_{name} hle_{name};\n#define {name} hle_{name}\n'
for phase,selected in [('global',global_names),('instance',instance_names),('device',set(functions)-global_names-instance_names)]:
    argument={'global':'void','instance':'VkInstance instance','device':'VkDevice device'}[phase]
    dispatch+=f'static int hle_load_{phase}({argument}) {{\n'
    if phase=='instance':
        dispatch+='hle_gdpa=(PFN_vkGetDeviceProcAddr)vk_icdGetInstanceProcAddr(instance,"vkGetDeviceProcAddr");if(!hle_gdpa)return 0;\n'
    for name in sorted(selected):
        loader='hle_gdpa(device,' if phase=='device' else 'vk_icdGetInstanceProcAddr('+('instance,' if phase=='instance' else 'VK_NULL_HANDLE,')
        dispatch+=f'hle_{name}=(PFN_{name}){loader}"{name}");if(!hle_{name})return 0;\n'
    dispatch+='return 1;\n}\n'
(root/'src/gpu_shaders/hle_probe_dispatch.h').write_text(dispatch)
