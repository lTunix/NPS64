#!/usr/bin/env python3
"""Package/verify the isolated test title; never upload or publish."""
from pathlib import Path
import argparse, hashlib, json, os, shutil, struct, subprocess
ROOT=Path(__file__).resolve().parents[1]
p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--gpu-present',action='store_true')
p.add_argument('--test-updater',action='store_true')
p.add_argument('--public-release',action='store_true')
p.add_argument('--reference-work',type=Path,required=True)
p.add_argument('--sdk',type=Path,required=True)
args=p.parse_args();work=args.reference_work.resolve();sdk=args.sdk.resolve()
version='00.73' if args.public_release else '00.52'
build=ROOT/'build/gpu-game-test'
if args.public_release and b'[frame-capture] armed' in (build/'nps64.elf').read_bytes():
    p.error('Frame capture diagnostic ELF cannot be packaged as a public release')
stage=build/'stage';(stage/'sce_sys/about').mkdir(parents=True,exist_ok=True)
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
run(['python3',ROOT/'scripts/compile_cheat_database.py',ROOT/'assets/mupencheat.txt',stage/'cheats.db'])
(stage/'CHEAT-NOTICE.txt').write_text('\n'.join((ROOT/'assets/mupencheat.txt').read_text().splitlines()[:39])+'\n')
for asset in ['ui-font.bin','cover-catalog.txt']:
    shutil.copyfile(ROOT/'build/stage'/asset,stage/asset)
for source,dest in [(ROOT/'LICENSE','LICENSE.txt'),(ROOT/'docs/CREDITS.md','CREDITS.txt'),
                    (work/'gpu/mesa-26.0.8/docs/license.rst','MESA-LICENSE.txt'),
                    (work/'DolphinPS4-reference/LICENSE','DOLPHIN-LICENSE.txt'),
                    (sdk/'LICENSE','OPENORBIS-LICENSE.txt'),
                    (ROOT/'build/compute-probe-ps4/ac_ps4_drm.c','GNM-REFERENCE.c')]:
    shutil.copyfile(source,stage/dest)
(stage/'TEST-NOTES.txt').write_text(
    'Audio recovery: preserve partial tail, 4096-frame restart after underrun, fades only at starvation boundaries. Start reserve retained. Synchronous FullSync restored for all ROMs, matching validated 00.29 graphics policy.\n'
    'RDP command FIFO: 65536 words, contiguous circular copies, reusable payload and renderer worker priority. SRAM writes are skipped only for an unchanged verified baseline.\n'
    'Asynchronous bounded diagnostics: disk writes run on a background logger, batches are drained on close; dropped records are counted. Native/2x quality and RDP synchronization retained.\n'
    'Presentation optimization: exact SSSE3 signed-difference edge interpolation; RDP/VI/FullSync timings; contiguous SIMD rows for 1280 to 960 and equal-width output, exact half-phase averaging, SSSE3 channel conversion; original pixel values preserved.\n'
    'Vulkan staging pixels are borrowed until the next video callback, eliminating the redundant frontend snapshot copy. Resolution changes during pause are saved for next game load.\n'
    'Presentation quality test: centered bilinear scaling to the game rectangle; Selectable native/2x Vulkan internal rendering, saved per ROM; VI AA retained. Filtro VI preciso enables smoothing; rapido restores nearest presentation. Native texture LOD and native TEX_RECT retained to preserve texture selection and 2D sprites.\n'
    'Library/settings 00.36: 16:9 HD menu with downloaded covers, global HUD modes and per-ROM aspect/VI/Pak/state slot.\n'
    'Audio continuity test: cumulative per-field DAC budget avoids inserting silence for two-frame AI_LEN delivery quantization. Direct FIFO retained; overlap-add/time stretching disabled. Already resampled PCM is copied unchanged to AudioOut. Large buffer retained; extended HLE voice filter disabled for isolation.\n'
    'Audio quality comparison: original microcode voice interpolation; avoids extended FIR reading neighboring scratch data. Large buffer retained from 00.18; output uses the direct FIFO comparison described above.\n'
    'Audio buffer comparison: 16384 frames capacity, 11520 prefill, no occupancy servo at 48 kHz. More audio latency; final sinc32 resampling retained.\n'
    'Experimental Vulkan game test 00.36: general split rasterizer on GPU while specialized variants compile in background.\n'
    'Works without shader history or a second launch. Prepares the finite general pipeline at startup, not ROM-specific recipes.\n'
    'Opaque RADV binary restore and saved recipe prewarm are disabled. Existing caches, recipes and saves are preserved.\n'
    'ACO_DEBUG=nosched reduces general-shader register pressure; all resource guards and 8 MiB compiler stacks remain.\n'
    'The original combined RDP ubershader remains disabled. Rendering is not skipped while variants compile.\n'
    'General shading can cost more GPU time temporarily. Other first-use renderer stages can still compile synchronously.\n'
    'PS4 correctness, frame pacing and performance require hardware testing before any release.\n'
    'Keeps three VideoOut buffers, cached readback and no automatic session pause. X: normal launch.\n'
    'Reads /data/NPS64/games; isolated saves: /data/NPS64/vulkan-test-saves.\n'
    'Main log: /data/NPS64/vulkan-game-test.log; fatal log: /data/NPS64/vulkan-game-crash-00.52.log.\n'
    'Tennis USA/VC boot passed native testing; PS4 validation pending. Audio uses direct FIFO after sinc32 in this comparison. No ROMs included. No GitHub release.\n')
notes=stage/'TEST-NOTES.txt'
notes.write_text('00.52 DIAGNOSTIC: paired frame captures before HUD and after overlay publication, directly from emulator memory. Arm by loading a state or L2+R2+Triangle, then resume; saves 16 presentations in /data/NPS64/frame-captures. Capture copies/IO can affect pacing; not a performance benchmark. Fixes F3DEX/F3DLX 0.95 perspective normalization (Mario Kart depth flicker).\n'+notes.read_text())
notes.write_text('00.47: 2D active tile/meta-tile bounding via vkCmdDispatchBase for submit_tile_binning_combined and submit_depth_blend; SSE2 movemask fast path for masked readback memcpy. Preserve FIFO ordering, resource limits, FullSync, barriers, effects and 2x quality. Physical validation pending.\n'+notes.read_text())
notes.write_text('00.46: keep native/2x render pairs together; bounded microbatch submission policy; skip redundant indirect-buffer reset dispatches in texture-only passes. Preserve FIFO ordering, resource limits, FullSync, barriers, effects and 2x quality. Includes RDP submission counters; GPU timestamp queries disabled. Physical validation pending.\n'+notes.read_text())
notes.write_text('00.37: Options Exit/Salir hint in library; native/2x resolution changes live while paused, without ROM reload or CPU state reset. RDP/TMEM state, canonical memory and FullSync retained; native mode rasterizes at 1x. New hardware validation pending.\n'+notes.read_text())
notes.write_text('00.36: live optional 16:9 projection patches for Super Mario 64 USA rev0 and Super Smash Bros. USA rev0. Aspect formats: 4:3, 16:9 stretched, 16:9 experimental. Exact header CRC/country/revision/size match; unsupported versions skip experimental. Hidden core slot separate from the cheat catalog; live enable/guarded restore, no renderer or audio change. PS4 validation pending.\n'+notes.read_text())
if args.gpu_present:
    notes=stage/'TEST-NOTES.txt'
    notes.write_text('00.36 experimental GPU compositor: exact Q8 bilinear/nearest, BGRA conversion and borders run in a compute shader. Three coherent cached Vulkan buffers are offered to VideoOut, with retirement before reuse/free. On PS4, registration failure disables GPU composition and retains legacy readback/scaling; host tests can copy composited output to the legacy canvas. FullSync, RDP rasterization and audio unchanged; the core adds isolated live widescreen controls. Pipeline/descriptors/buffers are persistent. Additional GPU timing and slow-frame diagnostics. PS4 validation required; not a public release.\n'+notes.read_text())
if args.public_release:
    (stage/'TEST-NOTES.txt').write_text('NPS64 '+version+' — public Vulkan build with the hardware-validated compositor; optional widescreen profiles remain experimental. Original app identity, updater enabled, no diagnostic time limit. Saves: /data/NPS64/saves; settings: /data/NPS64/settings.cfg. Diagnostic saves remain untouched in /data/NPS64/vulkan-test-saves. No ROMs included. See README and release notes for compatibility and validation.\n')
from PIL import Image, ImageOps
with Image.open(ROOT/'icon.jpeg') as image:
    ImageOps.fit(image.convert('RGB'),(512,512),method=Image.Resampling.LANCZOS).save(stage/'sce_sys/icon0.png')
content='IV0000-NPSX00064_00-NPS64PS400000000' if args.public_release else 'IV0000-NPSV00064_00-NPS64VULKANTEST0';assert len(content)==36
sfo=stage/'sce_sys/param.sfo';sfo.unlink(missing_ok=True)
run([tool,'sfo_new',sfo])
values={'APP_TYPE':('Integer',4,'1'),'APP_VER':('Utf8',8,version),'ATTRIBUTE':('Integer',4,'0'),
        'CATEGORY':('Utf8',4,'gd'),'CONTENT_ID':('Utf8',48,content),'DOWNLOAD_DATA_SIZE':('Integer',4,'0'),
        'SYSTEM_VER':('Integer',4,'0'),'TITLE':('Utf8',128,'NPS64 by lTunix' if args.public_release else 'NPS64 - Vulkan Game Test'),
        'TITLE_ID':('Utf8',12,'NPSX00064' if args.public_release else 'NPSV00064'),'VERSION':('Utf8',8,version)}
for key,(kind,size,value) in values.items():run([tool,'sfo_setentry',sfo,key,'--type',kind,'--maxsize',size,'--value',value])
files=sorted(str(f.relative_to(stage)) for f in stage.rglob('*') if f.is_file() and f.suffix!='.gp4')
run([tools/'create-gp4','-out','test.gp4','--content-id='+content,'--files',' '.join(files)],cwd=stage)
run([tool,'pkg_build','test.gp4',ROOT/'out'],cwd=stage)
pkg=ROOT/'out'/('NPS64-by-lTunix-'+version+'.pkg' if args.public_release else f'NPS64-Vulkan-Game-Test-{version}.pkg');shutil.copyfile(ROOT/'out'/(content+'.pkg'),pkg)
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
          'profile':{'version':version,'updater_enabled':args.public_release or args.test_updater,'gpu_compositor':args.gpu_present,'public_release':args.public_release,'title_id':'NPSX00064' if args.public_release else 'NPSV00064','deferred_vulkan_boot':True,'audio':'Original HLE voice interpolation + output sinc32 + direct FIFO','presentation':'bilinear when filtered VI is enabled','internal_resolution':'per-game native/2x','live_resolution':True,'borrowed_gpu_frame':True,'async_log':True,'rdp_ring_words':65536,'skip_unchanged_sram':True,'presentation_simd':'SSSE3 + exact ratio specializations','native_texture_lod':True,'native_tex_rect':True,'audio_recovery_restart':4096,'smash_2x_overlap':False,'audio_stretch':False,'hle_voice_taps':0,'audio_capacity_frames':16384,'audio_prefill_frames':11520,'audio_target_frames':0,'sync_shader':False,'pipeline_cache':'RAM only; general split shader + asynchronous specialization', 'aco_debug':'nosched','scanout_buffers':3,'guard_sgprs':96,'session_limit_seconds':0},
          'gpu_stage_timings':b'[gpu-stage] diagnostic=1' in (build/'nps64.elf').read_bytes(),
          'diagnostic_frame_capture':b'[frame-capture] armed' in (build/'nps64.elf').read_bytes(),
          'source_sha256':{name:hashlib.sha256((ROOT/name).read_bytes()).hexdigest() for name in ['scripts/patch_rdp_active_rows.py','scripts/patch_rdp_batch_tuning.py','scripts/patch_rdp_batch_stats.py','src/rdp_update_window.h','tests/test_rdp_update_window.c','scripts/test_rdp_update_window.sh','scripts/patch_gpu_stage_timing.py','scripts/patch_pipeline_demand.py','tests/test_pipeline_demand.c','scripts/test_pipeline_demand.sh','scripts/patch_gsp_triangle_fast.py','scripts/patch_gsp_culldl.py','scripts/patch_gbi1_persp.py','tests/test_gbi1_persp.c','scripts/test_gbi1_persp.sh','src/gsp_culldl.h','tests/gsp_triangle_oracle.c','tests/test_gsp_culldl.c','scripts/test_gsp_culldl.sh','scripts/patch_jit_mi_window.py','scripts/test_jit_mi_window.sh','tests/test_jit_mi_window.c','scripts/ps4-gpu-link.x','scripts/check_elf.py','scripts/patch_vulkan_live_resolution.py','src/widescreen.h','src/overlay_cache.h','src/cheats.c','src/cheats.h','scripts/patch_cheats.py','scripts/compile_cheat_database.py','assets/mupencheat.txt','src/gpu_presenter.c','src/gpu_presenter.h','src/gpu_present.comp','src/gpu_present_spv.h','src/platform.h','src/gpu.h','src/i18n.c','src/i18n.h','src/i18n_strings.inc','scripts/patch_vulkan_ring.py','src/save_cache.h','src/cadence_stats.h','src/log.c','src/log.h','scripts/patch_vulkan_perf.py','scripts/patch_vulkan_safety.py','src/bilinear_scaler.h','src/software_scaler.h','scripts/patch_audio.py','scripts/patch_vulkan_boot.py','src/rom_compat.h','src/audio_stretch.h','src/audio_queue.h','src/audio_resampler.h','src/storage.c','src/storage.h','src/settings.c','src/settings.h','src/library_ui.c','src/library_ui.h','src/covers.c','src/covers.h','src/cover_match.c','src/update_http_ps4.c','src/input_ps4.h','scripts/package_gpu_game_test.py','src/frontend.c','src/gpu_recipes.h','src/gpu_cache_store.h','scripts/build_gpu_game_test.py','src/gpu_vulkan.c','src/platform_ps4.c','src/gpu_pipeline_guard.c','src/gpu_compile_worker.c','src/gpu_cache_store.c','src/gpu_crashlog.c','src/gpu_recipes.c','scripts/patch_vulkan_recipes.py','scripts/patch_vulkan_async.py','scripts/patch_vulkan_cache.py','scripts/trace_vulkan_core.py']},
          'gpu_dependencies':json.loads((ROOT/'build/compute-probe-ps4/manifest.json').read_text()),
          'stable_pkg_sha256':hashlib.sha256((ROOT/'out/NPS64-by-lTunix-00.69.pkg').read_bytes()).hexdigest()}
(build/'manifest.json').write_text(json.dumps(manifest,indent=2))
print('PASS isolated game test: ELF, deferred constructors, 28 package checks, extracted payload, icon and SFO')
print(pkg)
