"""Embed the PS4-validated binning variant without probe-only readback writes."""
from pathlib import Path
import re,struct,subprocess
root=Path('/work');out=root/'build/parallel-game'
out.mkdir(parents=True,exist_ok=True)
subprocess.run(['python3','/work/scripts/prepare_tile_direct_probe.py'],check=True)
source=(root/'build/parallel-tile-probe/tile-direct_predicated.comp').read_text()
source,n=re.subn(r'layout\(set = 0, binding = 2, std430\) buffer InputTrace \{ uvec4 scissor_trace\[\]; \};','',source)
assert n==1
source,n=re.subn(r'    if \(local_index == 0u && group_index \* 32u < uint\(fb_info.primitive_count\)\)\n        scissor_trace\[group_index\] = uvec4\(scissor_state.elems\[group_index \* 32u\]\);\n','',source)
assert n==1 and 'scissor_trace' not in source
source=source.replace('void main()', '#if !UBERSHADER\nlayout(set=0, binding=2, std430) readonly buffer StateIndicesBuffer { InstanceIndicesMem elems[]; } state_indices;\n#endif\nvoid main()',1)
original=(root/'vendor/parallel-n64/mupen64plus-video-paraLLEl/parallel-rdp/parallel-rdp/shaders/tile_binning_combined.comp').read_text()
tail=original[original.index('#if !UBERSHADER\n    uint bit_count = uint(bitCount(binned_mask));',original.index('#else\n#if !UBERSHADER\n    uint bit_count')):]
tail=tail.replace('\n#endif\n#endif\n\n#if !UBERSHADER','\n#endif\n\n#if !UBERSHADER',1)
tail=tail.replace('int primitive_index = group_index * 32 + bit;', 'int primitive_index = int(group_index) * 32 + bit;')
source=source.rstrip()[:-1]+'\n'+tail.replace('binned_mask','mask')
path=out/'tile-game-predicated.comp';path.write_text(source)
includes=root/'vendor/parallel-n64/mupen64plus-video-paraLLEl/parallel-rdp/parallel-rdp/shaders'
spv=out/'tile-game-predicated.spv'
subprocess.run([str(root/'build/gpu/glslang-host/StandAlone/glslang'),'-V','--target-env','vulkan1.1',
 '-I'+str(includes),'-DUBERSHADER=1','-DSMALL_TYPES=0','-DSUBGROUP=0','-DDEBUG_ENABLE=0',str(path),'-o',str(spv)],check=True)
subprocess.run(['spirv-val','--target-env','vulkan1.1',str(spv)],check=True)
words=struct.unpack('<%dI'%(spv.stat().st_size//4),spv.read_bytes())
header='#pragma once\n#include <stdint.h>\n#ifdef __cplusplus\nnamespace RDP {\n#endif\nstatic const uint32_t nps64_tile_predicated_spirv[]={\n'
header+='\n'.join(','.join(f'0x{w:08x}u' for w in words[i:i+8])+',' for i in range(0,len(words),8))+'\n};\n#ifdef __cplusplus\n}\n#endif\n'
(out/'nps64_tile_predicated.hpp').write_text(header)
subprocess.run([str(root/'build/gpu/glslang-host/StandAlone/glslang'),'-V','--target-env','vulkan1.1',
 '-I'+str(includes),'-DUBERSHADER=0','-DSMALL_TYPES=0','-DSUBGROUP=0','-DDEBUG_ENABLE=0',str(path),'-o',str(out/'tile-game-split.spv')],check=True)
subprocess.run(['spirv-val','--target-env','vulkan1.1',str(out/'tile-game-split.spv')],check=True)
split=(out/'tile-game-split.spv').read_bytes();words=struct.unpack('<%dI'%(len(split)//4),split)
header=header[:header.index('static const uint32_t')]+ 'static const uint32_t nps64_tile_split_spirv[]={\n'
header+='\n'.join(','.join(f'0x{w:08x}u' for w in words[i:i+8])+',' for i in range(0,len(words),8))+'\n};\n#ifdef __cplusplus\n}\n#endif\n'
(out/'nps64_tile_split.hpp').write_text(header)
print('PASS game binning SPIR-V; diagnostic readback binding removed')
