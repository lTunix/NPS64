#!/usr/bin/env python3
"""Build isolated paraLLEl-RDP game test after physical compute verification."""
from pathlib import Path
import argparse, hashlib, json, os, shlex, shutil, subprocess
ROOT=Path(__file__).resolve().parents[1]
p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--reference-work',type=Path,default=ROOT.parent/'xuperPS4/.ps4-work')
p.add_argument('--sdk',type=Path,default=ROOT/'toolchain/OpenOrbis/PS4Toolchain')
p.add_argument('--gpu-present',action='store_true',help='Experimental GPU compositor and Vulkan-owned VideoOut buffers')
p.add_argument('--test-updater',action='store_true',help='Exercise production update status in the isolated test app')
p.add_argument('--public-release',action='store_true',help='Build the original app identity with updater and no diagnostic session cap')
p.add_argument('--frame-captures',action='store_true',help='Diagnostic paired frame captures from the emulator; not for releases')
p.add_argument('--stage-timings',action='store_true',help='Diagnostic GPU timestamp queries; never for public releases')
p.add_argument('--core-only',action='store_true')
p.add_argument('--link-only',action='store_true')
p.add_argument('--resume-core',action='store_true')
args=p.parse_args()
if args.stage_timings and args.public_release:p.error("Stage timing diagnostics cannot be a public release")
if args.frame_captures and args.public_release:p.error("Frame capture diagnostics cannot be a public release")
# Direct presentation was validated on PS4 in diagnostic 00.32.
if args.public_release:args.gpu_present=True
work=args.reference_work.resolve();sdk=args.sdk.resolve();gpu=work/'gpu'
build=ROOT/'build/gpu-game-test';core=build/'core';build.mkdir(parents=True,exist_ok=True)
env=dict(os.environ,OO_PS4_TOOLCHAIN=str(sdk),DOTNET_SYSTEM_GLOBALIZATION_INVARIANT='1')
env['LD_LIBRARY_PATH']=str(work/'llvm/usr/lib/x86_64-linux-gnu')+':'+env.get('LD_LIBRARY_PATH','')
env['PATH']=str(ROOT/'build/tools/usr/bin')+':'+env['PATH']
def run(command,cwd=ROOT):subprocess.run(list(map(str,command)),env=env,cwd=cwd,check=True)
base=['--target=x86_64-pc-freebsd12-elf','-fPIC','-funwind-tables','-D__ORBIS__','-D__PS4__',
      '-D_GNU_SOURCE','-isysroot',str(sdk),'-isystem',str(sdk/'include'),
      '-isystem',str(gpu/'sysroot/libc-overlay'),'-isystem',str(work/'DolphinPS4-reference/port/mesa-overlay')]
cc=work/'llvm/usr/bin/clang-18';cxx=work/'llvm/usr/bin/clang++-18'
for name,compiler,extra in [('cc',cc,[]),('cxx',cxx,['-nostdinc++','-isystem',str(sdk/'include/c++/v1'),'-std=c++17'])]:
    wrapper=build/name
    wrapper.write_text('#!/bin/sh\nexec '+shlex.join([str(compiler),*base,*extra])+' "$@"\n')
    wrapper.chmod(0o755)
if args.link_only:
    run(['python3',ROOT/'scripts/patch_pipeline_demand.py',core])
    run(['python3',ROOT/'scripts/patch_gpu_stage_timing.py',core])
    run(['python3',ROOT/'scripts/patch_rdp_batch_stats.py',core])
    run(['python3',ROOT/'scripts/patch_rdp_active_rows.py',core])
    run(['python3',ROOT/'scripts/patch_rdp_game_compat.py',core])
    run(['python3',ROOT/'scripts/patch_rdp_batch_tuning.py',core])
    run(['python3',ROOT/'scripts/patch_jit_mi_window.py',core])
    run(['python3',ROOT/'scripts/patch_jit_delay_entry.py',core])
    run(['python3',ROOT/'scripts/patch_gsp_culldl.py',core])
    run(['python3',ROOT/'scripts/patch_gbi1_persp.py',core])
    run(['python3',ROOT/'scripts/patch_gbi2_xbus.py',core])
    run(['python3',ROOT/'scripts/patch_vertex_screen_fast.py',core])
    run(['python3',ROOT/'scripts/patch_rsp_div_normalize.py',core])
    run(['python3',ROOT/'scripts/patch_gsp_triangle_fast.py',core])
    run(['python3',ROOT/'scripts/patch_hle_cpu_timing.py',core])
    run(['python3',ROOT/'scripts/patch_hle_backend_init.py',core])
    run(['python3',ROOT/'scripts/patch_cheats.py',core])
    run(['python3',ROOT/'scripts/patch_vulkan_live_resolution.py',core])
    # The cheat API is exposed by this frontend: never link an older unsafe parser.
    patch_make=['make','-C',core,'platform=unix','ARCH=x86_64','WITH_DYNAREC=x86_64','WANT_HACKTARUX=0','HAVE_ARI64_X64=1','HAVE_OPENGL=0','HAVE_PARALLEL=1','HAVE_PARALLEL_RSP=0','HAVE_THR_AL=1','STATIC_LINKING=1','TARGET=parallel_n64.a','CPUOPTS=-O3 -march=btver2 -mtune=btver2 -DNDEBUG -DNPS64_COMPRESSED_MEMORY','GIT_VERSION=890323d','CC='+str(build/'cc'),'CXX='+str(build/'cxx'),'AR=ar']
    run([*patch_make,'-j4'])
if not args.link_only:
    if not args.resume_core:
        shutil.copytree(ROOT/'vendor/parallel-n64',core,dirs_exist_ok=True,ignore=shutil.ignore_patterns('.git'))
        run(['python3',ROOT/'scripts/patch_core.py',core])
    run(['python3',ROOT/'scripts/patch_jit_mi_window.py',core])
    run(['python3',ROOT/'scripts/patch_jit_delay_entry.py',core])
    run(['python3',ROOT/'scripts/patch_gsp_culldl.py',core])
    run(['python3',ROOT/'scripts/patch_gbi1_persp.py',core])
    run(['python3',ROOT/'scripts/patch_gbi2_xbus.py',core])
    run(['python3',ROOT/'scripts/patch_vertex_screen_fast.py',core])
    run(['python3',ROOT/'scripts/patch_rsp_div_normalize.py',core])
    run(['python3',ROOT/'scripts/patch_gsp_triangle_fast.py',core])
    run(['python3',ROOT/'scripts/patch_hle_cpu_timing.py',core])
    run(['python3',ROOT/'scripts/patch_hle_backend_init.py',core])
    run(['python3',ROOT/'scripts/patch_vulkan_boot.py',core])
    run(['python3',ROOT/'scripts/patch_vulkan_safety.py',core])
    run(['python3',ROOT/'scripts/patch_vulkan_cache.py',core])
    run(['python3',ROOT/'scripts/patch_vulkan_recipes.py',core])
    run(['python3',ROOT/'scripts/patch_vulkan_async.py',core])
    run(['python3',ROOT/'scripts/patch_vulkan_perf.py',core])
    run(['python3',ROOT/'scripts/patch_pipeline_demand.py',core])
    run(['python3',ROOT/'scripts/patch_gpu_stage_timing.py',core])
    run(['python3',ROOT/'scripts/patch_rdp_batch_stats.py',core])
    run(['python3',ROOT/'scripts/patch_rdp_active_rows.py',core])
    run(['python3',ROOT/'scripts/patch_rdp_game_compat.py',core])
    run(['python3',ROOT/'scripts/patch_rdp_batch_tuning.py',core])
    run(['python3',ROOT/'scripts/patch_vulkan_ring.py',core])
    run(['python3',ROOT/'scripts/patch_cheats.py',core])
    run(['python3',ROOT/'scripts/patch_vulkan_live_resolution.py',core])
    run(['python3',ROOT/'scripts/trace_vulkan_core.py',core])
    shutil.copyfile(ROOT/'src/rom_compat.h',core/'mupen64plus-core/src/main/nps64_rom_compat.h')
    make=['make','-C',core,'platform=unix','ARCH=x86_64','WITH_DYNAREC=x86_64','WANT_HACKTARUX=0',
          'HAVE_ARI64_X64=1','HAVE_OPENGL=0','HAVE_PARALLEL=1','HAVE_PARALLEL_RSP=0','HAVE_THR_AL=1',
          'STATIC_LINKING=1','TARGET=parallel_n64.a','CPUOPTS=-O3 -march=btver2 -mtune=btver2 -DNDEBUG -DNPS64_COMPRESSED_MEMORY',
          'GIT_VERSION=890323d','CC='+str(build/'cc'),'CXX='+str(build/'cxx'),'AR=ar']
    run([*make,'asm-defines-regen'])
    run([*make,'-j4'])
if args.core_only:raise SystemExit(0)
probe=ROOT/'build/compute-probe-ps4'
manifest=json.loads((probe/'manifest.json').read_text())
for filename,digest in manifest['library_members'].items():
    if hashlib.sha256(Path(filename).read_bytes()).hexdigest()!=digest:
        raise SystemExit('Tested GPU library changed: '+filename)
libraries=[Path(x) for x in json.loads((gpu/'libraries.json').read_text())]
libraries=[probe/'libamd_common.a' if x.name=='libamd_common.a' else x for x in libraries]
objects=[]
for unit in [*(['frame_capture'] if args.frame_captures else []),'cheats','i18n','settings','library_ui','covers','cover_match','update_http_ps4',*(['updater','update_feed'] if args.public_release or args.test_updater else []),'frontend',*(['gpu_presenter'] if args.gpu_present else []),'gpu_vulkan','gpu_pipeline_guard','gpu_compile_worker','gpu_api_trace','gpu_cache_store','gpu_recipes','gpu_crashlog','sha256','gpu_boot','platform_ps4','storage','runtime_ps4','log']:
    obj=build/(unit+'.o')
    stack_check=['-Wframe-larger-than=4096','-fstack-usage'] if unit in {'gpu_pipeline_guard','gpu_compile_worker','gpu_cache_store','gpu_recipes','gpu_crashlog','sha256'} else []
    run([build/'cc',*(['-DNPS64_FRAME_CAPTURE_TEST'] if args.frame_captures else []),*(['-DNPS64_RDP_STAGE_TIMINGS'] if args.stage_timings else []),*(['-DNPS64_UPDATES'] if args.test_updater else []),*(['-DNPS64_GPU_PRESENT_TEST'] if args.gpu_present else []),*(['-DNPS64_PUBLIC_RELEASE','-DNPS64_UPDATES','-DNPS64_UI_VERSION="00.74"'] if args.public_release else ['-DNPS64_UI_VERSION="00.57"']),'-DNPS64_GPU','-DNPS64_GPU_GAME_TEST','-DNPS64_AUDIO_BUFFER_TEST','-DNPS64_AUDIO_HQ_TEST','-DNPS64_AUDIO_CLEAN_TEST','-DNPS64_IMAGE_SMOOTH_TEST','-DNPS64_INTERNAL_2X_TEST','-DNPS64_GPU_BORROW_TEST','-DNPS64_AUDIO_DIRECT_TEST','-DNPS64_AUDIO_RECOVERY_TEST','-DNPS64_HD_UI','-std=c11','-O3','-march=btver2','-mtune=btver2','-Wall','-Wextra','-Werror',
         *stack_check,*(['-DNPS64_LOG_ASYNC'] if unit=='log' else []),*(['-mssse3'] if unit=='frontend' else []),*(['-Wno-unused-function'] if unit=='covers' else []),
         '-I'+str(ROOT/'src'),'-I'+str(core/'libretro-common/include'),
         '-I'+str(core/'mupen64plus-core/src/api'),
         '-I'+str(core/'mupen64plus-video-paraLLEl/parallel-rdp/vulkan-headers/include'),
         '-c',ROOT/('src/'+unit+'.c'),'-o',obj]);objects.append(obj)
for unit,extra in [('ps4_malloc',['-DNPS64_GPU','-DNPS64_GPU_GAME_TEST']),('ps4_dlmalloc',[])]:
    obj=build/(unit+'.o');run([build/'cc',*(['-DNPS64_FRAME_CAPTURE_TEST'] if args.frame_captures else []),*(['-DNPS64_RDP_STAGE_TIMINGS'] if args.stage_timings else []),*extra,'-O2','-c',ROOT/('src/third_party/'+unit+'.c'),'-o',obj]);objects.append(obj)
run([work/'llvm/usr/bin/ld.lld-18','-m','elf_x86_64','-pie','-z','max-page-size=0x4000','--script',
     ROOT/'scripts/ps4-gpu-link.x','--eh-frame-hdr','--wrap=pthread_once','--wrap=mprotect',
     '-L'+str(sdk/'lib'),sdk/'lib/crt1.o',*objects,'--start-group',core/'parallel_n64.a',
     '--whole-archive',*libraries,'--no-whole-archive','-lc++','-lc++abi','-lunwind','-lc','-lm','-lkernel',
     '-lSceLibcInternal','-lSceGnmDriver','-lSceVideoOut','-lScePad','-lSceUserService','-lSceAudioOut',
     '-lSceNet','-lSceSsl','-lSceHttp','-lSceSystemService','-lSceSysmodule','--end-group','-o',build/'nps64.elf'])
run(['python3',ROOT/'scripts/package_gpu_game_test.py','--reference-work',work,'--sdk',sdk,*(['--public-release'] if args.public_release else []),*(['--gpu-present'] if args.gpu_present else []),*(['--test-updater'] if args.test_updater else [])])
