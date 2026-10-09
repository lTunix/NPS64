"""Generate a reproducible compute shader and static RADV dispatch table."""
from pathlib import Path
import re
import struct
import subprocess

root = Path('/work')
out = root / 'build/parallel-probe'
out.mkdir(parents=True, exist_ok=True)
spv = out / 'probe.spv'
subprocess.run([
    str(root / 'build/gpu/glslang-host/StandAlone/glslang'),
    '-V', '--target-env', 'vulkan1.1',
    str(root / 'src/gpu_shaders/parallel_compute_probe.comp'), '-o', str(spv)
], check=True)
subprocess.run(['spirv-val', '--target-env', 'vulkan1.1', str(spv)], check=True)
words = struct.unpack('<%dI' % (spv.stat().st_size // 4), spv.read_bytes())
header = '#pragma once\n#include <stdint.h>\nstatic const uint32_t parallel_probe_spv[]={\n'
for i in range(0, len(words), 8):
    header += ','.join('0x%08xu' % word for word in words[i:i + 8]) + ',\n'
header += '};\n'
(out / 'parallel_probe_spv.h').write_text(header)

source = (root / 'src/parallel_compute_probe.c').read_text()
functions = set(re.findall(r'\b(vk[A-Z]\w+)\s*\(', source))
global_names = {'vkCreateInstance'}
instance_names = {
    'vkDestroyInstance', 'vkEnumeratePhysicalDevices', 'vkGetPhysicalDeviceProperties',
    'vkGetPhysicalDeviceQueueFamilyProperties', 'vkGetPhysicalDeviceMemoryProperties',
    'vkCreateDevice',
}
assert global_names | instance_names <= functions
dispatch = '#pragma once\nextern PFN_vkVoidFunction vk_icdGetInstanceProcAddr(VkInstance,const char*);\n'
dispatch += 'static PFN_vkGetDeviceProcAddr parallel_gdpa;\n'
for name in sorted(functions):
    dispatch += f'static PFN_{name} parallel_{name};\n#define {name} parallel_{name}\n'
for phase, selected in [
    ('global', global_names), ('instance', instance_names),
    ('device', functions - global_names - instance_names),
]:
    argument = {'global': 'void', 'instance': 'VkInstance instance', 'device': 'VkDevice device'}[phase]
    dispatch += f'static int parallel_load_{phase}({argument}) {{\n'
    if phase == 'instance':
        dispatch += 'parallel_gdpa=(PFN_vkGetDeviceProcAddr)vk_icdGetInstanceProcAddr(instance,"vkGetDeviceProcAddr");if(!parallel_gdpa)return 0;\n'
    for name in sorted(selected):
        loader = 'parallel_gdpa(device,' if phase == 'device' else \
            'vk_icdGetInstanceProcAddr(' + ('instance,' if phase == 'instance' else 'VK_NULL_HANDLE,')
        dispatch += f'parallel_{name}=(PFN_{name}){loader}"{name}");if(!parallel_{name})return 0;\n'
    dispatch += 'return 1;\n}\n'
(out / 'parallel_probe_dispatch.h').write_text(dispatch)
print('PASS SPIR-V validated and PS4 static Vulkan dispatch generated')
