"""Generate static RADV entry points from the probe's Vulkan calls."""
from pathlib import Path
import re
root=Path('/work');out=root/'build/parallel-tile-probe'
source=(root/'src/parallel_tile_probe.c').read_text()
functions=set(re.findall(r'\b(vk[A-Z]\w+)\s*\(',source))
global_names={'vkCreateInstance'}
instance_names={'vkDestroyInstance','vkEnumeratePhysicalDevices','vkGetPhysicalDeviceProperties',
 'vkGetPhysicalDeviceFeatures2','vkGetPhysicalDeviceQueueFamilyProperties',
 'vkGetPhysicalDeviceMemoryProperties','vkCreateDevice'}
assert global_names|instance_names<=functions
dispatch='#pragma once\nextern PFN_vkVoidFunction vk_icdGetInstanceProcAddr(VkInstance,const char*);\nstatic PFN_vkGetDeviceProcAddr parallel_gdpa;\n'
for name in sorted(functions):dispatch+=f'static PFN_{name} parallel_{name};\n#define {name} parallel_{name}\n'
for phase,selected in [('global',global_names),('instance',instance_names),('device',functions-global_names-instance_names)]:
 argument={'global':'void','instance':'VkInstance instance','device':'VkDevice device'}[phase]
 dispatch+=f'static int parallel_load_{phase}({argument}){{\n'
 if phase=='instance':dispatch+='parallel_gdpa=(PFN_vkGetDeviceProcAddr)vk_icdGetInstanceProcAddr(instance,"vkGetDeviceProcAddr");if(!parallel_gdpa)return 0;\n'
 for name in sorted(selected):
  loader='parallel_gdpa(device,' if phase=='device' else 'vk_icdGetInstanceProcAddr('+('instance,' if phase=='instance' else 'VK_NULL_HANDLE,')
  dispatch+=f'parallel_{name}=(PFN_{name}){loader}"{name}");if(!parallel_{name})return 0;\n'
 dispatch+='return 1;\n}\n'
(out/'parallel_probe_dispatch.h').write_text(dispatch)
