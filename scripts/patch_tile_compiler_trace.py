#!/usr/bin/env python3
"""Opt-in, allocation-free PS4 compute compiler stage markers."""
from pathlib import Path
import sys

path = Path(sys.argv[1])
source = path.read_text()
if 'nps64_cs_compile_trace' in source:
    raise SystemExit(0)
anchor = 'struct radv_shader *\nradv_compile_cs('
assert source.count(anchor) == 1
helper = '''static void
nps64_cs_compile_trace(const char *stage)
{
#ifdef __ORBIS__
   if (getenv("NPS64_TILE_COMPILER_TRACE")) {
      extern void nps64_boot_trace(const char *);
      nps64_boot_trace(stage);
   }
#else
   (void)stage;
#endif
}

'''
source = source.replace(anchor, helper + anchor)
markers = [
 ('   /* Compile SPIR-V shader to NIR. */', 'compiler SPIRV -> NIR begin'),
 ('   radv_optimize_nir(cs_stage->nir,', 'compiler NIR optimize begin'),
 ('   /* Run the shader info pass. */', 'compiler shader info begin'),
 ('   radv_declare_shader_args(device,', 'compiler shader args begin'),
 ('   /* Postprocess NIR. */', 'compiler NIR postprocess begin'),
 ('   bool dump_shader = radv_can_dump_shader', 'compiler NIR postprocess complete'),
 ('   /* Compile NIR shader to AMD assembly. */', 'compiler ACO begin'),
 ('   /* Dump NIR after nir_to_asm,', 'compiler ACO complete'),
 ('   cs_shader = radv_shader_create(', 'compiler shader allocation/upload begin'),
 ('   cs_shader->nir_string = nir_string;', 'compiler shader allocation/upload complete'),
 ('   return cs_shader;', 'compiler compute complete'),
]
for anchor, label in markers:
    assert source.count(anchor) == 1, anchor
    source = source.replace(anchor, f'   nps64_cs_compile_trace("{label}");\n' + anchor)
path.write_bytes(source.encode())
