#!/usr/bin/env python3
"""Package/verify the isolated test title; never upload or publish."""
from pathlib import Path
import argparse, hashlib, json, os, shutil, struct, subprocess
ROOT=Path(__file__).resolve().parents[1]
p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--reference-work',type=Path,required=True)
p.add_argument('--sdk',type=Path,required=True)
args=p.parse_args();work=args.reference_work.resolve();sdk=args.sdk.resolve()
build=ROOT/'build/gpu-game-test';stage=build/'stage';(stage/'sce_sys/about').mkdir(parents=True,exist_ok=True)
(stage/'sce_module').mkdir(exist_ok=True)
env=dict(os.environ,OO_PS4_TOOLCHAIN=str(sdk),DOTNET_SYSTEM_GLOBALIZATION_INVARIANT='1')
env['LD_LIBRARY_PATH']=str(work/'llvm/usr/lib/x86_64-linux-gnu')+':'+env.get('LD_LIBRARY_PATH','')
def run(command,cwd=ROOT,stdout=None):subprocess.run(list(map(str,command)),cwd=cwd,env=env,check=True,stdout=stdout)
tools=sdk/'bin/linux';tool=tools/'PkgTool.Core'
run([tools/'create-fself','-in='+str(build/'nps64.elf'),'-out='+str(build/'nps64.oelf'),
     '--eboot',stage/'eboot.bin','--paid','0x3800000000000011'])
run(['python3',ROOT/'scripts/check_elf.py',build])
run(['python3',ROOT/'scripts/check_gpu_boot.py',build/'nps64.elf'])
for name in ['sce_module/libc.prx','sce_module/libSceFios2.prx','sce_sys/about/right.sprx']:
    shutil.copyfile(sdk/'samples/piglet'/name,stage/name)
shutil.copyfile(ROOT/'assets/font.bin',stage/'font.bin')
for asset in ['ui-font.bin','cover-catalog.txt']:
    shutil.copyfile(ROOT/'build/stage'/asset,stage/asset)
for source,dest in [(ROOT/'LICENSE','LICENSE.txt'),(ROOT/'docs/CREDITS.md','CREDITS.txt'),
                    (work/'gpu/mesa-26.0.8/docs/license.rst','MESA-LICENSE.txt'),
                    (work/'DolphinPS4-reference/LICENSE','DOLPHIN-LICENSE.txt'),
                    (sdk/'LICENSE','OPENORBIS-LICENSE.txt'),
                    (ROOT/'build/compute-probe-ps4/ac_ps4_drm.c','GNM-REFERENCE.c')]:
    shutil.copyfile(source,stage/dest)
(stage/'TEST-NOTES.txt').write_text(
    'Presentation optimization: contiguous SIMD rows for 1280 to 960 and equal-width output, exact half-phase averaging, SSSE3 channel conversion; original pixel values preserved.\n'
    'Vulkan staging pixels are borrowed until the next video callback, eliminating the redundant frontend snapshot copy. Resolution changes during pause are saved for next game load.\n'
    'Presentation quality test: centered bilinear scaling to the game rectangle; Selectable native/2x Vulkan internal rendering, saved per ROM; VI AA retained. Filtro VI preciso enables smoothing; rapido restores nearest presentation. Native texture LOD and native TEX_RECT retained to preserve texture selection and 2D sprites.\n'
    'Library/settings 00.26: 16:9 HD menu with downloaded covers, global HUD modes and per-ROM aspect/VI/Pak/state slot.\n'
    'Audio continuity test: cumulative per-field DAC budget avoids inserting silence for two-frame AI_LEN delivery quantization. Direct FIFO retained; overlap-add/time stretching disabled. Already resampled PCM is copied unchanged to AudioOut. Large buffer retained; extended HLE voice filter disabled for isolation.\n'
    'Audio quality comparison: original microcode voice interpolation; avoids extended FIR reading neighboring scratch data. Large buffer retained from 00.18; output uses the direct FIFO comparison described above.\n'
    'Audio buffer comparison: 16384 frames capacity, 11520 prefill, no occupancy servo at 48 kHz. More audio latency; final sinc32 resampling retained.\n'
    'Experimental Vulkan game test 00.26: general split rasterizer on GPU while specialized variants compile in background.\n'
    'Works without shader history or a second launch. Prepares the finite general pipeline at startup, not ROM-specific recipes.\n'
    'Opaque RADV binary restore and saved recipe prewarm are disabled. Existing caches, recipes and saves are preserved.\n'
    'ACO_DEBUG=nosched reduces general-shader register pressure; all resource guards and 8 MiB compiler stacks remain.\n'
    'The original combined RDP ubershader remains disabled. Rendering is not skipped while variants compile.\n'
    'General shading can cost more GPU time temporarily. Other first-use renderer stages can still compile synchronously.\n'
    'PS4 correctness, frame pacing and performance require hardware testing before any release.\n'
    'Keeps three VideoOut buffers, cached readback and 600-second session pause. X: normal launch.\n'
    'Reads /data/NPS64/games; isolated saves: /data/NPS64/vulkan-test-saves.\n'
    'Main log: /data/NPS64/vulkan-game-test.log; fatal log: /data/NPS64/vulkan-game-crash-00.26.log.\n'
    'Tennis USA/VC boot passed native testing; PS4 validation pending. Audio uses direct FIFO after sinc32 in this comparison. No ROMs included. No GitHub release.\n')
from PIL import Image, ImageOps
with Image.open(ROOT/'icon.jpeg') as image:
    ImageOps.fit(image.convert('RGB'),(512,512),method=Image.Resampling.LANCZOS).save(stage/'sce_sys/icon0.png')
content='IV0000-NPSV00064_00-NPS64VULKANTEST0';assert len(content)==36
sfo=stage/'sce_sys/param.sfo';sfo.unlink(missing_ok=True)
run([tool,'sfo_new',sfo])
values={'APP_TYPE':('Integer',4,'1'),'APP_VER':('Utf8',8,'00.26'),'ATTRIBUTE':('Integer',4,'0'),
        'CATEGORY':('Utf8',4,'gd'),'CONTENT_ID':('Utf8',48,content),'DOWNLOAD_DATA_SIZE':('Integer',4,'0'),
        'SYSTEM_VER':('Integer',4,'0'),'TITLE':('Utf8',128,'NPS64 - Vulkan Game Test'),
        'TITLE_ID':('Utf8',12,'NPSV00064'),'VERSION':('Utf8',8,'00.26')}
for key,(kind,size,value) in values.items():run([tool,'sfo_setentry',sfo,key,'--type',kind,'--maxsize',size,'--value',value])
files=sorted(str(f.relative_to(stage)) for f in stage.rglob('*') if f.is_file() and f.suffix!='.gp4')
run([tools/'create-gp4','-out','test.gp4','--content-id='+content,'--files',' '.join(files)],cwd=stage)
run([tool,'pkg_build','test.gp4',ROOT/'out'],cwd=stage)
pkg=ROOT/'out/NPS64-Vulkan-Game-Test-00.26.pkg';shutil.copyfile(ROOT/'out'/(content+'.pkg'),pkg)
with (build/'pkg-validation.log').open('w') as stream:run([tool,'pkg_validate','--verbose',pkg],stdout=stream)
checks=[x for x in (build/'pkg-validation.log').read_text().splitlines() if x.startswith('[')]
assert len(checks)>20 and all(x.startswith('[OK]') for x in checks)
with (build/'pkg-extraction.log').open('w') as stream:run([tool,'pkg_extract',pkg,build/'extracted'],stdout=stream)
for name in files:
    if name in ['sce_sys/param.sfo','sce_sys/icon0.png']:continue
    assert (stage/name).read_bytes()==(build/'extracted/uroot'/name).read_bytes(),name
with (build/'entries.log').open('w') as stream:run([tool,'pkg_listentries',pkg],stdout=stream)
data=pkg.read_bytes();entries=(build/'entries.log').read_text().splitlines()
def entry(name):
    row=next(x.split() for x in entries if x.endswith(' '+name));offset,size=int(row[0],16),int(row[1],16)
    assert offset+size<=len(data)
    return data[offset:offset+size]
assert entry('ICON0_PNG')==(stage/'sce_sys/icon0.png').read_bytes()
def fields(data):
    magic,version,keys,values,count=struct.unpack_from('<IIIII',data);assert magic==0x46535000
    result={}
    for i in range(count):
        key,kind,length,capacity,value=struct.unpack_from('<HHIII',data,20+i*16)
        result[data[keys+key:].split(b'\0')[0]]=data[values+value:values+value+length]
    return result
staged=fields(sfo.read_bytes());packaged=fields(entry('PARAM_SFO'));assert all(packaged.get(k)==v for k,v in staged.items())
sha=hashlib.sha256(data).hexdigest();pkg.with_suffix('.sha256').write_text(sha+'  '+pkg.name+'\n')
manifest={'package_sha256':sha,'bytes':len(data),'core_sha256':hashlib.sha256((build/'core/parallel_n64.a').read_bytes()).hexdigest(),
          'profile':{'version':'00.26','deferred_vulkan_boot':True,'audio':'Original HLE voice interpolation + output sinc32 + direct FIFO','presentation':'bilinear when filtered VI is enabled','internal_resolution':'per-game native/2x','borrowed_gpu_frame':True,'presentation_simd':'SSSE3 + exact ratio specializations','native_texture_lod':True,'native_tex_rect':True,'audio_stretch':False,'hle_voice_taps':0,'audio_capacity_frames':16384,'audio_prefill_frames':11520,'audio_target_frames':0,'sync_shader':False,'pipeline_cache':'RAM only; general split shader + asynchronous specialization', 'aco_debug':'nosched','scanout_buffers':3,'guard_sgprs':96,'session_limit_seconds':600},
          'source_sha256':{name:hashlib.sha256((ROOT/name).read_bytes()).hexdigest() for name in ['scripts/patch_vulkan_safety.py','src/bilinear_scaler.h','src/software_scaler.h','scripts/patch_audio.py','scripts/patch_vulkan_boot.py','src/rom_compat.h','src/audio_stretch.h','src/audio_queue.h','src/audio_resampler.h','src/storage.c','src/storage.h','src/settings.c','src/settings.h','src/library_ui.c','src/library_ui.h','src/covers.c','src/covers.h','src/cover_match.c','src/update_http_ps4.c','src/input_ps4.h','scripts/package_gpu_game_test.py','src/frontend.c','src/gpu_recipes.h','src/gpu_cache_store.h','scripts/build_gpu_game_test.py','src/gpu_vulkan.c','src/platform_ps4.c','src/gpu_pipeline_guard.c','src/gpu_compile_worker.c','src/gpu_cache_store.c','src/gpu_crashlog.c','src/gpu_recipes.c','scripts/patch_vulkan_recipes.py','scripts/patch_vulkan_async.py','scripts/patch_vulkan_cache.py','scripts/trace_vulkan_core.py']},
          'gpu_dependencies':json.loads((ROOT/'build/compute-probe-ps4/manifest.json').read_text()),
          'stable_pkg_sha256':hashlib.sha256((ROOT/'out/NPS64-by-lTunix-00.69.pkg').read_bytes()).hexdigest()}
(build/'manifest.json').write_text(json.dumps(manifest,indent=2))
print('PASS isolated game test: ELF, deferred constructors, 28 package checks, extracted payload, icon and SFO')
print(pkg)
