"""Build a semantically equivalent depth/blend entry point without early return.

PS4 experiment only: this does not establish a compiler defect.
"""
from pathlib import Path
import struct
import subprocess

root = Path('/work')
out = root / 'build/parallel-game'
includes = root / 'vendor/parallel-n64/mupen64plus-video-paraLLEl/parallel-rdp/parallel-rdp/shaders'
source = (includes / 'depth_blend.comp').read_text()
anchor = '    if (coarse_binned == 0u)\n        return;'
assert source.count(anchor) == 1
source = source.replace(anchor, '    if (coarse_binned != 0u)\n    {', 1)
source = source.rstrip()[:-1] + '    }\n}\n'
source = source.replace('#version 450', '#version 450\n#extension GL_GOOGLE_include_directive : require', 1)
path = out / 'depth-blend-predicated.comp'
path.write_text(source)
spv = out / 'depth-blend-predicated.spv'
subprocess.run([str(root/'build/gpu/glslang-host/StandAlone/glslang'), '-V',
    '--target-env', 'vulkan1.1', '-I'+str(includes), '-DSMALL_TYPES=0',
    '-DSUBGROUP=0', '-DDEBUG_ENABLE=0', str(path), '-o', str(spv)], check=True)
subprocess.run(['spirv-val', '--target-env', 'vulkan1.1', str(spv)], check=True)
data = spv.read_bytes()
words = struct.unpack('<%dI' % (len(data)//4), data)
header = '#pragma once\n#include <stdint.h>\nnamespace RDP {\nstatic const uint32_t nps64_blend_predicated_spirv[]={\n'
header += '\n'.join(','.join(f'0x{w:08x}u' for w in words[i:i+8])+','
    for i in range(0, len(words), 8)) + '\n};\n}\n'
(out/'nps64_blend_predicated.hpp').write_text(header)
print('PASS depth/blend entry-point SPIR-V; early return replaced by conditional block')
