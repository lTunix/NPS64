"""Build an isolated original tile dispatch from external PS4 hang captures.
Never adds captured ROM-derived bytes to tracked sources. References are
computed on host Vulkan and checked for full 32/64-lane equivalence.
"""
from pathlib import Path
import hashlib,json,os,re,struct,subprocess,sys

root=Path('/work');capture=Path(sys.argv[1] if len(sys.argv)>1 else '/work/logs')
out=root/'build/parallel-tile-probe';out.mkdir(parents=True,exist_ok=True)
subprocess.run([sys.executable,str(root/'scripts/prepare_tile_direct_probe.py')],check=True)
includes=root/'vendor/parallel-n64/mupen64plus-video-paraLLEl/parallel-rdp/parallel-rdp/shaders'
report=(capture/'gpu-hang.txt').read_text()
assert 'memory 0x5 done' in report, 'Expected tile-stage marker 5 capture'
assert 'COMPUTE_DIM_X' in report and 'COMPUTE_DIM_Y' in report
clean=re.sub(r'\x1b\[[0-9;]*m','',report)
assert re.search(r'COMPUTE_NUM_THREAD_X.*<- 32\b',clean), 'Review original tile group size'
assert 'COMPUTE_USER_DATA_3 = 00000140' in clean and 'COMPUTE_USER_DATA_4 = 000000ef' in clean and 'COMPUTE_USER_DATA_5 = 00000001' in clean
triangle=(capture/'gpu-hang-data-43efe20000.bin').read_bytes()
scissor=(capture/'gpu-hang-data-43efe2e000.bin').read_bytes()
assert len(triangle)==8192 and len(scissor)==4096
assert struct.unpack_from('<4I',scissor)==(0,0,1280,960), 'Review captured scissor'
source=(includes/'tile_binning_combined.comp').read_text()
assert source.count('layout(local_size_x = 32) in;')==1
manifest={'kind':'isolated tile stage, post-hang inputs, no preceding span stage',
          'resolution':[320,239],'primitives':1,'constants':[32,8,8,256,1024,32768,1],
          'inputs':{name:hashlib.sha256(data).hexdigest() for name,data in
                    [('triangle',triangle),('scissor',scissor)]},'source_sha256':hashlib.sha256(source.encode()).hexdigest()}
for local in (32,64):
    glsl=source.replace('#version 450','#version 450\n#extension GL_GOOGLE_include_directive : require',1)
    glsl=glsl.replace('layout(local_size_x = 32) in;',f'layout(local_size_x = {local}) in;')
    path=out/f'tile-{local}.comp';path.write_text(glsl)
    subprocess.run([str(root/'build/gpu/glslang-host/StandAlone/glslang'),'-V','--target-env','vulkan1.1',
        '-I'+str(includes),'-DUBERSHADER=1','-DSMALL_TYPES=0','-DSUBGROUP=0','-DDEBUG_ENABLE=0',
        str(path),'-o',str(out/f'tile-{local}.spv')],check=True)
    subprocess.run(['spirv-val','--target-env','vulkan1.1',str(out/f'tile-{local}.spv')],check=True)

def array(name,data,sparse=False):
    values=struct.unpack('<%dI'%(len(data)//4),data)
    result=f'static const uint32_t {name}[{len(values)}]={{\n'
    if sparse:
        result+='\n'.join(f'[{i}]=0x{v:08x}u,' for i,v in enumerate(values) if v)
    else:
        result+='\n'.join(','.join(f'0x{v:08x}u' for v in values[i:i+8])+',' for i in range(0,len(values),8))
    return result+'\n};\n'
def header(mask,coarse):
    text='#pragma once\n#include <stdint.h>\n'
    text+=array('tile_triangle',triangle,True)+array('tile_scissor',scissor,True)
    for local in (32,64):text+=array(f'tile_spv{local}',(out/f'tile-{local}.spv').read_bytes())
    text+=array('tile_expected_mask',mask,True)+array('tile_expected_coarse',coarse,True)
    (out/'parallel_tile_data.h').write_text(text)

subprocess.run([sys.executable,str(root/'scripts/prepare_tile_dispatch.py')],check=True)

def compile_host():
    subprocess.run(['clang-21','-std=c11','-O2','-Wall','-Wextra','-Werror','-DNPS64_PARALLEL_TILE_PROBE',
        '-I'+str(root/'src'),'-I'+str(out),str(root/'src/parallel_tile_probe.c'),
        str(root/'tests/parallel_tile_host.c'),'-lvulkan','-o',str(out/'host')],check=True)
env=dict(os.environ,VK_ICD_FILENAMES='/usr/share/vulkan/icd.d/lvp_icd.x86_64.json',
         VK_INSTANCE_LAYERS='VK_LAYER_KHRONOS_validation',
         VK_LAYER_ENABLES='VK_VALIDATION_FEATURE_ENABLE_SYNCHRONIZATION_VALIDATION_EXT')
env.pop('NPS64_TILE_RECORD',None)
header(bytes(524288),bytes(65536));compile_host()
with (out/'record.log').open('w') as log:
    subprocess.run([str(out/'host')],env=dict(env,NPS64_TILE_RECORD=str(out/'reference')),stdout=log,stderr=subprocess.STDOUT,check=True,timeout=60)
mask=(out/'reference-mask.bin').read_bytes();coarse=(out/'reference-coarse.bin').read_bytes()
assert len(mask)==524288 and len(coarse)==65536
assert any(mask) and any(coarse), 'All-zero reference would not exercise the real tile input'
# This first primitive covers the full captured 320x239 area. Check the
# independent rectangular tile expectation before using host GPU as reference.
active={y*128+x for y in range(30) for x in range(40)}
coarse_words=struct.unpack('<16384I',coarse);mask_words=struct.unpack('<131072I',mask)
assert all(v==(1 if i in active else 0) for i,v in enumerate(coarse_words))
assert all(v==(1 if i//8 in active and i%8==0 else 0) for i,v in enumerate(mask_words))
manifest['reference_sha256']={'mask':hashlib.sha256(mask).hexdigest(),'coarse':hashlib.sha256(coarse).hexdigest()}
header(mask,coarse);compile_host()
with (out/'host.log').open('w') as log:
    subprocess.run([str(out/'host')],env=env,stdout=log,stderr=subprocess.STDOUT,check=True,timeout=90)
log=(out/'host.log').read_text()
assert 'VUID-' not in log and 'Validation Error' not in log
assert log.count('COMPLETE case=')==384 and log.count('PASS case=')==6
(out/'manifest.json').write_text(json.dumps(manifest,indent=2)+'\n')
print('PASS original tile32/tile64 and direct diagnostic variants: 384 fences; all masks, inputs and guards exact; no Vulkan validation errors; host only')
