"""Opt-in patch of a prepared core build copy; upstream stays unchanged."""
from pathlib import Path
import sys
root=Path(sys.argv[1])/'mupen64plus-video-paraLLEl/parallel-rdp/parallel-rdp'
(root/'shaders/nps64_tile_predicated.hpp').write_bytes(Path('/work/build/parallel-game/nps64_tile_predicated.hpp').read_bytes())
(root/'shaders/nps64_tile_split.hpp').write_bytes(Path('/work/build/parallel-game/nps64_tile_split.hpp').read_bytes())
path=root/'rdp_renderer.hpp';s=path.read_text()
assert s.count('bool direct_binning = false;')==1
s=s.replace('bool direct_binning = false;','bool direct_binning = false;\n\t\tbool predicated_binning = false;');path.write_text(s)
path=root/'rdp_renderer.cpp';s=path.read_text()
s=s.replace('#include "shaders/nps64_tile_direct.hpp"','#include "shaders/nps64_tile_direct.hpp"\n#include "shaders/nps64_tile_predicated.hpp"\n#include "shaders/nps64_tile_split.hpp"',1)
anchor='\tbool allow_subgroup = true;';assert s.count(anchor)==1
s=s.replace(anchor,'\tcaps.predicated_binning = getenv("NPS64_RDP_PREDICATED_BINNING");\n\tif (caps.predicated_binning) LOGI("NPS64 predicated tile binning: local=32, no shared LDS, coarse atomics; split=%d\\n", int(!caps.ubershader));\n'+anchor)
anchor='\tif (caps.direct_binning)\n\t{';assert s.count(anchor)==1
s=s.replace(anchor,'''\tif (caps.predicated_binning)
\t{
\t\tconst auto &layout = shader_bank->tile_binning_combined->get_shader(Vulkan::ShaderStage::Compute)->get_layout();
\t\tif (caps.ubershader)
\t\t\tcmd.set_program(device->request_program(nps64_tile_predicated_spirv, sizeof(nps64_tile_predicated_spirv), &layout));
\t\telse
\t\t\tcmd.set_program(device->request_program(nps64_tile_split_spirv, sizeof(nps64_tile_split_spirv), &layout));
\t\tsubgroup_size = 32;
\t}
\telse if (caps.direct_binning)
\t{''')
path.write_text(s)
