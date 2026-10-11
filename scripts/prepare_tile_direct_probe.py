"""Diagnostic tile variants; retain original fixture/reference unchanged."""
from pathlib import Path
import struct, subprocess, re
root=Path('/work');out=root/'build/parallel-tile-probe'
includes=root/'vendor/parallel-n64/mupen64plus-video-paraLLEl/parallel-rdp/parallel-rdp/shaders'
source=(includes/'tile_binning_combined.comp').read_text()
prefix=source[:source.index('#if !SUBGROUP\nshared uint merged_mask_shared;')]
prefix=prefix.replace('#version 450','#version 450\n#extension GL_GOOGLE_include_directive : require',1)
body='''
void main()
{
    uint group_index = gl_WorkGroupID.x;
    uint local_index = gl_LocalInvocationIndex;
    ivec2 tile = ivec2(gl_WorkGroupID.y * 8u + (local_index & 7u),
                      gl_WorkGroupID.z * 4u + (local_index >> 3u));
    ivec2 base_coord = tile * ivec2(TILE_WIDTH, TILE_HEIGHT);
    ivec2 end_coord = min(base_coord + ivec2(TILE_WIDTH, TILE_HEIGHT),
                         ivec2(fb_info.resolution)) - 1;
    uint mask = 0u;
    for (uint bit = 0u; bit < 32u; bit++) {
        uint primitive = group_index * 32u + bit;
        if (primitive >= uint(fb_info.primitive_count)) break;
        ScissorState scissor = load_scissor_state(primitive);
        TriangleSetup setup = load_triangle_setup(primitive);
        if (bin_primitive(setup, base_coord, end_coord, SCALE_FACTOR, scissor))
            mask |= 1u << bit;
    }
    int linear_tile = tile.y * MAX_TILES_X + tile.x;
    binned_bitmask[linear_tile * TILE_BINNING_STRIDE + int(group_index)] = mask;
    COARSE_WRITE
}
'''
header='#pragma once\n#include <stdint.h>\n'
for name,coarse in [
 ('direct_atomic','if (mask != 0u) atomicOr(binned_bitmask_coarse[linear_tile], 1u << group_index); else atomicAnd(binned_bitmask_coarse[linear_tile], ~(1u << group_index));'),
 # Unique writer only because the isolated fixture dispatches ONE primitive group.
 ('direct_store','binned_bitmask_coarse[linear_tile] = mask != 0u ? 1u : 0u;'),
 ('direct_volatile','if (mask != 0u) atomicOr(binned_bitmask_coarse[linear_tile], 1u << group_index); else atomicAnd(binned_bitmask_coarse[linear_tile], ~(1u << group_index));')]:
 glsl=prefix+body.replace('COARSE_WRITE',coarse)
 if name=='direct_volatile':
  glsl=glsl.replace('readonly buffer TriangleSetupBuffer','volatile readonly buffer TriangleSetupBuffer').replace('readonly buffer ScissorStateBuffer','volatile readonly buffer ScissorStateBuffer')
  glsl,n=re.subn(r'layout\(set = 0, binding = 2, std430\) readonly buffer StateIndicesBuffer\s*\{.*?\}\s*state_indices;',
   'layout(set = 0, binding = 2, std430) buffer InputTrace { uvec4 scissor_trace[]; };',glsl,flags=re.S)
  assert n==1
  glsl=glsl.replace('    uint mask = 0u;', '    if (local_index == 0u && group_index * 32u < uint(fb_info.primitive_count))\n        scissor_trace[group_index] = uvec4(scissor_state.elems[group_index * 32u]);\n    uint mask = 0u;')
 path=out/f'tile-{name}.comp';path.write_text(glsl)
 spv=out/f'tile-{name}.spv'
 subprocess.run([str(root/'build/gpu/glslang-host/StandAlone/glslang'),'-V','--target-env','vulkan1.1',
  '-I'+str(includes),'-DUBERSHADER=1','-DSMALL_TYPES=0','-DSUBGROUP=0','-DDEBUG_ENABLE=0',str(path),'-o',str(spv)],check=True)
 subprocess.run(['spirv-val','--target-env','vulkan1.1',str(spv)],check=True)
 words=struct.unpack('<%dI'%(spv.stat().st_size//4),spv.read_bytes())
 header+=f'static const uint32_t tile_spv_{name}[{len(words)}]={{\n'
 header+='\n'.join(','.join(f'0x{w:08x}u' for w in words[i:i+8])+',' for i in range(0,len(words),8))+'\n};\n'
(out/'parallel_tile_direct_data.h').write_text(header)
