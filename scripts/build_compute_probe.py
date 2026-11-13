#!/usr/bin/env python3
"""Build a separate PS4 compute test using the prepared xuperPS4 Mesa tree.

Does not rebuild/modify xuperPS4 or the stable NPS64 stage and does not upload.
The exact archive members used are hashed into the build manifest.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import shlex
import shutil
import struct
import subprocess

ROOT = Path(__file__).resolve().parents[1]
p = argparse.ArgumentParser(description=__doc__)
p.add_argument('--reference-work', type=Path, default=ROOT.parent/'xuperPS4/.ps4-work')
p.add_argument('--sdk', type=Path, default=ROOT/'toolchain/OpenOrbis/PS4Toolchain')
p.add_argument('--compile-only', action='store_true', help='Build 00.02 compiler/statistics test, with no compute submissions')
p.add_argument('--multiwave', action='store_true', help='Build 00.03 bounded runtime-mask LDS test at 32/64/128/256 threads')
p.add_argument('--guard-compare', action='store_true', help='Build 00.04: compile pipeline 52bb with and without the resource guard; no submissions')
args = p.parse_args()
if args.guard_compare:args.compile_only=True
if args.compile_only and args.multiwave:p.error('Select one test mode')
work, sdk = args.reference_work.resolve(), args.sdk.resolve()
gpu = work/'gpu'
build = ROOT/('build/guard-compare-ps4' if args.guard_compare else 'build/pipeline-compile-ps4' if args.compile_only else 'build/multiwave-probe-ps4' if args.multiwave else 'build/compute-probe-ps4')
stage = build/'stage'
stage.mkdir(parents=True, exist_ok=True)
(stage/'sce_sys/about').mkdir(parents=True, exist_ok=True)
(stage/'sce_module').mkdir(exist_ok=True)
(ROOT/'out').mkdir(exist_ok=True)
env = dict(os.environ, DOTNET_SYSTEM_GLOBALIZATION_INVARIANT='1', OO_PS4_TOOLCHAIN=str(sdk))
env['LD_LIBRARY_PATH'] = str(work/'llvm/usr/lib/x86_64-linux-gnu')+':'+env.get('LD_LIBRARY_PATH','')

def run(command, cwd=ROOT):
    subprocess.run(list(map(str, command)), cwd=cwd, env=env, check=True)

def digest(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()

revision = subprocess.check_output(['git','-C',str(work/'DolphinPS4-reference'),'rev-parse','HEAD'],text=True).strip()
if revision != '69360a158a9619d7c877d4229c2b37b0a34982dc':
    raise SystemExit('Unexpected DolphinPS4 revision; review GPU dependencies first')
libraries = [Path(x) for x in json.loads((gpu/'libraries.json').read_text())]
for library in libraries:
    if not library.is_file():
        raise SystemExit('Missing Mesa library: '+str(library))

# Resolve exactly the entry points used by the shared host/PS4 test. Never
# depend on the weak Linux loader symbols in a statically linked PS4 binary.
test_source='tests/compile_captured_pipeline.c' if args.compile_only else 'tests/gpu_compute_probe.c'
source = (ROOT/test_source).read_text()
functions = set(re.findall(r'\b(vk[A-Z]\w+)\s*\(', source))
if args.compile_only:
    assert not functions & {'vkQueueSubmit','vkCmdDispatch','vkCmdDispatchIndirect','vkAllocateCommandBuffers','vkCreateCommandPool'}
    assert not any(name.startswith(('vkQueue','vkCmd')) for name in functions)
global_names = {'vkCreateInstance'}
instance_names = {'vkCreateDevice','vkDestroyInstance','vkEnumeratePhysicalDevices',
                  'vkGetPhysicalDeviceProperties','vkGetPhysicalDeviceQueueFamilyProperties',
                  'vkGetPhysicalDeviceMemoryProperties'}
if args.compile_only:
    instance_names={'vkCreateDevice','vkDestroyInstance','vkEnumeratePhysicalDevices',
                    'vkGetPhysicalDeviceProperties','vkGetPhysicalDeviceQueueFamilyProperties',
                    'vkGetPhysicalDeviceFeatures2','vkGetDeviceProcAddr'}
header = '#pragma once\nextern PFN_vkVoidFunction vk_icdGetInstanceProcAddr(VkInstance,const char*);\n'
header += 'static PFN_vkGetDeviceProcAddr probe_gdpa;\n'
for name in sorted(functions):
    header += f'static PFN_{name} probe_{name};\n#define {name} probe_{name}\n'
for phase, selected in [('global',global_names),('instance',instance_names),
                         ('device',functions-global_names-instance_names)]:
    argument = {'global':'void','instance':'VkInstance instance','device':'VkDevice device'}[phase]
    header += f'static int probe_load_{phase}({argument}) {{\n'
    if phase == 'instance':
        header += 'probe_gdpa=(PFN_vkGetDeviceProcAddr)vk_icdGetInstanceProcAddr(instance,"vkGetDeviceProcAddr");if(!probe_gdpa)return 0;\n'
    for name in sorted(selected):
        loader = 'probe_gdpa(device,' if phase=='device' else 'vk_icdGetInstanceProcAddr('+('instance,' if phase=='instance' else 'VK_NULL_HANDLE,')
        header += f'probe_{name}=(PFN_{name}){loader}"{name}");if(!probe_{name})return 0;\n'
    header += 'return 1;\n}\n'
(build/('pipeline_compile_dispatch.h' if args.compile_only else 'compute_probe_dispatch.h')).write_text(header)
if args.compile_only:
    cases=[]
    for group,folder in [('uber','bonaire-pipelines'),('split','bonaire-split')]:
        for path in sorted((ROOT/'build/vulkan-offline'/folder).glob('*.npcp')):
            if group=='uber' and path.stem not in {'5fb308f708ff1198','e03a18dca5918de5'}:continue
            name=group+'-'+path.name
            shutil.copyfile(path,stage/name);cases.append((group,name))
    assert sum(g=='uber' for g,n in cases)==2 and sum(g=='split' for g,n in cases)==23
    if args.guard_compare:
        source=stage/'split-52bbce7140cf109d.npcp'
        first=stage/'without-guard-52bbce7140cf109d.npcp'
        second=stage/'with-guard-52bbce7140cf109d.npcp'
        shutil.copyfile(source,first);shutil.copyfile(source,second)
        cases=[('uber',first.name),('split',second.name)]
        for path in stage.glob('*.npcp'):
            if path.name not in {first.name,second.name}:path.unlink()
    cases_header='#pragma once\nstatic const char *compile_cases[]={'+','.join(json.dumps('/app0/'+n) for g,n in cases)+'};\n'
    cases_header+='static const unsigned compile_groups[]={'+','.join('0' if g=='uber' else '1' for g,n in cases)+'};\n'
    (build/'compile_probe_cases.h').write_text(cases_header)
else:
    run([gpu/'tools/usr/bin/glslangValidator','-V','--target-env','vulkan1.1',
         ROOT/('tests/gpu_multiwave_probe.comp' if args.multiwave else 'tests/gpu_compute_probe.comp'),'-o',stage/'probe.spv'])
    run([gpu/'tools/usr/bin/spirv-val','--target-env','vulkan1.1',stage/'probe.spv'])

# Keep GNM diagnostic files in NPS64's own directory. Only paths differ from
# the xuperPS4 driver; rebuild this object without editing its source tree.
driver = gpu/'mesa-26.0.8/src/amd/common/ac_ps4_drm.c'
adapted = build/'ac_ps4_drm.c'
adapted.write_text(driver.read_text().replace('/data/xuperps4-gpu.log','/data/NPS64/gpu-probe-driver.log')
                   .replace('/data/xuperps4/gpu/','/data/NPS64/gpu-probe/'))
commands = json.loads((gpu/'mesa-build/compile_commands.json').read_text())
command = next(x for x in commands if x['file'].endswith('/ac_ps4_drm.c'))
words = shlex.split(command['command'])
words = [w.replace(str(work/'OpenOrbis/PS4Toolchain'),str(sdk)) for w in words]
for option, value in [('-o',build/'ac_ps4_drm.o'),('-MF',build/'ac_ps4_drm.o.d'),
                       ('-MQ',build/'ac_ps4_drm.o'),('-c',adapted)]:
    words[words.index(option)+1] = str(value)
run(words, cwd=command['directory'])
common = next(x for x in libraries if x.name=='libamd_common.a')
members = subprocess.check_output(['ar','t',str(common)],text=True).splitlines()
members = [str((common.parent/x).resolve()) for x in members if not x.endswith('/ac_ps4_drm.c.o')]
assert members
replacement = build/'libamd_common.a'
replacement.unlink(missing_ok=True)
run(['ar','rcs',replacement,*members,build/'ac_ps4_drm.o'])
libraries = [replacement if x==common else x for x in libraries]

cc = work/'llvm/usr/bin/clang-18'
flags = ['--target=x86_64-pc-freebsd12-elf','-fPIC','-funwind-tables','-D__ORBIS__',
         '-D__PS4__','-D_GNU_SOURCE','-DNPS64_COMPUTE_PROBE','-O2','-std=c11',
         '-Wall','-Wextra','-Werror','-isysroot',sdk,'-isystem',sdk/'include',
         '-I'+str(ROOT/'src'),'-I'+str(build),'-I'+str(gpu/'mesa-26.0.8/include')]
objects = []
if args.compile_only:flags+=['-DNPS64_PIPELINE_COMPILE_ONLY']
if args.guard_compare:flags+=['-DNPS64_PIPELINE_GUARD_TEST','-DNPS64_GUARD_COMPARE']
if args.multiwave:flags+=['-DNPS64_MULTIWAVE_PROBE']
frontend='src/pipeline_compile_frontend.c' if args.compile_only else 'src/compute_probe_frontend.c'
units=[frontend,test_source,'src/gpu_boot.c',
                 'src/platform_ps4.c','src/log.c','src/runtime_ps4.c',
                 'src/third_party/ps4_malloc.c','src/third_party/ps4_dlmalloc.c']
if args.guard_compare:units+=['src/gpu_pipeline_guard.c','src/gpu_compile_worker.c']
for filename in units:
    obj = build/(Path(filename).stem+'.o')
    extra = ['-DNPS64_GPU'] if filename.endswith(('platform_ps4.c','ps4_malloc.c')) else []
    # dlmalloc is third-party C with its own portability warnings.
    if filename.endswith('ps4_dlmalloc.c'):extra += ['-Wno-error']
    run([cc,*flags,*extra,'-c',ROOT/filename,'-o',obj])
    objects.append(obj)
ld = work/'llvm/usr/bin/ld.lld-18'
run([ld,'-m','elf_x86_64','-pie','-z','max-page-size=0x4000','--script',ROOT/'scripts/ps4-gpu-link.x',
     '--eh-frame-hdr','--wrap=pthread_once','--wrap=mprotect','-L'+str(sdk/'lib'),sdk/'lib/crt1.o',
     *objects,'--start-group','--whole-archive',*libraries,'--no-whole-archive',
     '-lc++','-lc++abi','-lunwind','-lc','-lm','-lkernel','-lSceLibcInternal',
     '-lSceGnmDriver','-lSceVideoOut','-lScePad','-lSceUserService','-lSceAudioOut',
     '-lSceSystemService','-lSceSysmodule','--end-group','-o',build/'nps64.elf'])
tools = sdk/'bin/linux'
run([tools/'create-fself','-in='+str(build/'nps64.elf'),'-out='+str(build/'nps64.oelf'),
     '--eboot',stage/'eboot.bin','--paid','0x3800000000000011'])
run(['python3',ROOT/'scripts/check_elf.py',build])
run(['python3',ROOT/'scripts/check_gpu_boot.py','--hle-probe',build/'nps64.elf'])
for name in ['sce_module/libc.prx','sce_module/libSceFios2.prx','sce_sys/about/right.sprx']:
    shutil.copyfile(sdk/'samples/piglet'/name,stage/name)
shutil.copyfile(ROOT/'assets/font.bin',stage/'font.bin')
shutil.copyfile(ROOT/'LICENSE',stage/'LICENSE.txt')
shutil.copyfile(gpu/'mesa-26.0.8/docs/license.rst',stage/'MESA-LICENSE.txt')
shutil.copyfile(work/'DolphinPS4-reference/LICENSE',stage/'DOLPHIN-LICENSE.txt')
shutil.copyfile(driver,stage/'GNM-REFERENCE.c')
(stage/'CREDITS.txt').write_text('NPS64 Vulkan compute diagnostic. Mesa 26.0.8; DolphinPS4 '+revision+
    '; prepared xuperPS4 01.19 GNM driver. The reference source retains its MIT notice.\n')
from PIL import Image, ImageOps
with Image.open(ROOT/'icon.jpeg') as icon:
    ImageOps.contain(icon.convert('RGB'),(512,512)).resize((512,512),Image.Resampling.LANCZOS).save(stage/'sce_sys/icon0.png')
content = 'IV0000-NPSG00064_00-NPS64GPUPROBE000'
assert len(content)==36
sfo = stage/'sce_sys/param.sfo'
sfo.unlink(missing_ok=True)
run([tools/'PkgTool.Core','sfo_new',sfo])
version='00.04' if args.guard_compare else '00.02' if args.compile_only else '00.03' if args.multiwave else '00.01'
title='NPS64 - Vulkan Guard Compare' if args.guard_compare else 'NPS64 - Vulkan Compile Test' if args.compile_only else 'NPS64 - Vulkan Multiwave Test' if args.multiwave else 'NPS64 - Vulkan Compute Test'
entries = {'APP_TYPE':('Integer',4,'1'),'APP_VER':('Utf8',8,version),
           'ATTRIBUTE':('Integer',4,'0'),'CATEGORY':('Utf8',4,'gd'),
           'CONTENT_ID':('Utf8',48,content),'DOWNLOAD_DATA_SIZE':('Integer',4,'0'),
           'SYSTEM_VER':('Integer',4,'0'),'TITLE':('Utf8',128,title),
           'TITLE_ID':('Utf8',12,'NPSG00064'),'VERSION':('Utf8',8,version)}
for key,(kind,size,value) in entries.items():
    run([tools/'PkgTool.Core','sfo_setentry',sfo,key,'--type',kind,'--maxsize',size,'--value',value])
files = sorted(str(x.relative_to(stage)) for x in stage.rglob('*') if x.is_file() and x.suffix!='.gp4')
run([tools/'create-gp4','-out','probe.gp4','--content-id='+content,'--files',' '.join(files)],cwd=stage)
run([tools/'PkgTool.Core','pkg_build','probe.gp4',ROOT/'out'],cwd=stage)
original = ROOT/'out'/(content+'.pkg')
package = ROOT/('out/NPS64-Vulkan-Guard-Compare-00.04.pkg' if args.guard_compare else 'out/NPS64-Vulkan-Compile-Test-00.02.pkg' if args.compile_only else 'out/NPS64-Vulkan-Multiwave-Test-00.03.pkg' if args.multiwave else 'out/NPS64-Vulkan-Compute-Test-00.01.pkg')
shutil.copyfile(original,package)
with (build/'pkg-validation.log').open('w') as log:
    subprocess.run([str(tools/'PkgTool.Core'),'pkg_validate','--verbose',str(package)],env=env,check=True,stdout=log)
checks = [x for x in (build/'pkg-validation.log').read_text().splitlines() if x.startswith('[')]
assert len(checks)>20 and all(x.startswith('[OK]') for x in checks)
with (build/'extraction.log').open('w') as log:
    subprocess.run([str(tools/'PkgTool.Core'),'pkg_extract',str(package),str(build/'extracted')],env=env,check=True,stdout=log)
for name in files:
    if name in {'sce_sys/param.sfo','sce_sys/icon0.png'}:continue
    assert (stage/name).read_bytes()==(build/'extracted/uroot'/name).read_bytes(),name
entries=subprocess.check_output([str(tools/'PkgTool.Core'),'pkg_listentries',str(package)],env=env,text=True)
def package_entry(name):
    row=next(x.split() for x in entries.splitlines() if x.endswith(' '+name))
    with package.open('rb') as stream:
        stream.seek(int(row[0],16));return stream.read(int(row[1],16))
assert package_entry('ICON0_PNG')==(stage/'sce_sys/icon0.png').read_bytes()
def sfo_fields(data):
    magic,fmt,keys,values,count=struct.unpack_from('<IIIII',data);assert magic==0x46535000
    result={}
    for index in range(count):
        key,kind,length,capacity,value=struct.unpack_from('<HHIII',data,20+index*16)
        result[data[keys+key:].split(b'\0')[0]]=data[values+value:values+value+length]
    return result
staged=sfo_fields(sfo.read_bytes());packaged=sfo_fields(package_entry('PARAM_SFO'))
assert all(packaged.get(key)==value for key,value in staged.items())
print('PASS extracted payload, packaged icon and SFO application fields')
manifest = {'dolphin_revision':revision,'reference_driver_sha256':digest(driver),
            'adapted_driver_sha256':digest(adapted),'package_sha256':digest(package),
            'package_bytes':package.stat().st_size,'library_members':{}}
for library in libraries:
    data = library.read_bytes()
    manifest['library_members'][str(library)] = digest(library)
    if data.startswith(b'!<thin>'):
        for member in subprocess.check_output(['ar','t',str(library)],text=True).splitlines():
            path = (library.parent/member).resolve()
            manifest['library_members'][str(path)] = digest(path)
(build/'manifest.json').write_text(json.dumps(manifest,indent=2))
(package.with_suffix('.sha256')).write_text(digest(package)+'  '+package.name+'\n')
print('PASS isolated PKG; integrity checks='+str(len(checks)))
print(package)
