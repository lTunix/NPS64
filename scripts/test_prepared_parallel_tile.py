"""Check preserved capture data when the external logs directory was replaced."""
from pathlib import Path
import hashlib,json,os,re,struct,subprocess
root=Path('/work');out=root/'build/parallel-tile-probe'
manifest=json.loads((out/'manifest.json').read_text());header=(out/'parallel_tile_data.h').read_text()
upstream=root/'vendor/parallel-n64/mupen64plus-video-paraLLEl/parallel-rdp/parallel-rdp/shaders/tile_binning_combined.comp'
assert hashlib.sha256(upstream.read_text().encode()).hexdigest()==manifest['source_sha256']
def data(name):
    match=re.search(r'static const uint32_t '+name+r'\[(\d+)\]=\{(.*?)\};',header,re.S)
    assert match,name
    words=[0]*int(match[1]);body=match[2]
    if '[' in body:
        for index,value in re.findall(r'\[(\d+)\]=0x([0-9a-f]+)u',body):words[int(index)]=int(value,16)
    else:words=[int(v,16) for v in re.findall(r'0x([0-9a-f]+)u',body)]
    assert len(words)==int(match[1]);return struct.pack('<%dI'%len(words),*words)
for name,key in [('tile_triangle','triangle'),('tile_scissor','scissor')]:
    assert hashlib.sha256(data(name)).hexdigest()==manifest['inputs'][key]
for name,key in [('tile_expected_mask','mask'),('tile_expected_coarse','coarse')]:
    assert hashlib.sha256(data(name)).hexdigest()==manifest['reference_sha256'][key]
for local in (32,64):
    assert data(f'tile_spv{local}')==(out/f'tile-{local}.spv').read_bytes()
    subprocess.run(['spirv-val','--target-env','vulkan1.1',str(out/f'tile-{local}.spv')],check=True)
subprocess.run(['clang-21','-std=c11','-O2','-Wall','-Wextra','-Werror','-DNPS64_PARALLEL_TILE_PROBE',
    '-I/work/src','-I'+str(out),'/work/src/parallel_tile_probe.c','/work/tests/parallel_tile_host.c',
    '-lvulkan','-o',str(out/'host')],check=True)
env=dict(os.environ,VK_ICD_FILENAMES='/usr/share/vulkan/icd.d/lvp_icd.x86_64.json',
    VK_INSTANCE_LAYERS='VK_LAYER_KHRONOS_validation',VK_LAYER_ENABLES='VK_VALIDATION_FEATURE_ENABLE_SYNCHRONIZATION_VALIDATION_EXT')
env.pop('NPS64_TILE_RECORD',None)
with (out/'host.log').open('w') as log:
    subprocess.run([str(out/'host')],env=env,stdout=log,stderr=subprocess.STDOUT,check=True,timeout=90)
log=(out/'host.log').read_text()
assert 'VUID-' not in log and 'Validation Error' not in log
assert log.count('COMPLETE case=')==128 and log.count('PASS case=')==2
assert log.count('COMPILE pipeline case=0 local=32 begin')==1
assert log.count('COMPILE pipeline case=1 local=64 begin')==1
print('PASS preserved capture hashes, SPIR-V, lazy pipeline compilation, 128 fences and complete guards; host only')
