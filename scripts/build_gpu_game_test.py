#!/usr/bin/env python3
"""Build isolated paraLLEl-RDP game test after physical compute verification."""
from pathlib import Path
import argparse, hashlib, json, os, shlex, shutil, subprocess
ROOT=Path(__file__).resolve().parents[1]
p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--reference-work',type=Path,default=ROOT.parent/'xuperPS4/.ps4-work')
p.add_argument('--sdk',type=Path,default=ROOT/'toolchain/OpenOrbis/PS4Toolchain')
p.add_argument('--core-only',action='store_true')
p.add_argument('--link-only',action='store_true')
p.add_argument('--resume-core',action='store_true')
args=p.parse_args()
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
if not args.link_only:
    if not args.resume_core:
        shutil.copytree(ROOT/'vendor/parallel-n64',core,dirs_exist_ok=True,ignore=shutil.ignore_patterns('.git'))
        run(['python3',ROOT/'scripts/patch_core.py',core])
    run(['python3',ROOT/'scripts/patch_vulkan_boot.py',core])
    run(['python3',ROOT/'scripts/patch_vulkan_safety.py',core])
    run(['python3',ROOT/'scripts/patch_vulkan_cache.py',core])
    run(['python3',ROOT/'scripts/patch_vulkan_recipes.py',core])
    run(['python3',ROOT/'scripts/patch_vulkan_async.py',core])
    run(['python3',ROOT/'scripts/trace_vulkan_core.py',core])
    shutil.copyfile(ROOT/'src/rom_compat.h',core/'mupen64plus-core/src/main/nps64_rom_compat.h')
    make=['make','-C',core,'platform=unix','ARCH=x86_64','WITH_DYNAREC=x86_64','WANT_HACKTARUX=0',
          'HAVE_ARI64_X64=1','HAVE_OPENGL=0','HAVE_PARALLEL=1','HAVE_PARALLEL_RSP=0','HAVE_THR_AL=1',
          'STATIC_LINKING=1','TARGET=parallel_n64.a','CPUOPTS=-O2 -DNDEBUG -DNPS64_COMPRESSED_MEMORY',
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
for unit in ['settings','library_ui','covers','cover_match','update_http_ps4','frontend','gpu_vulkan','gpu_pipeline_guard','gpu_compile_worker','gpu_api_trace','gpu_cache_store','gpu_recipes','gpu_crashlog','sha256','gpu_boot','platform_ps4','storage','runtime_ps4','log']:
    obj=build/(unit+'.o')
    stack_check=['-Wframe-larger-than=4096','-fstack-usage'] if unit in {'gpu_pipeline_guard','gpu_compile_worker','gpu_cache_store','gpu_recipes','gpu_crashlog','sha256'} else []
    run([build/'cc','-DNPS64_GPU','-DNPS64_GPU_GAME_TEST','-DNPS64_AUDIO_BUFFER_TEST','-DNPS64_AUDIO_HQ_TEST','-DNPS64_AUDIO_CLEAN_TEST','-DNPS64_IMAGE_SMOOTH_TEST','-DNPS64_INTERNAL_2X_TEST','-DNPS64_GPU_BORROW_TEST','-DNPS64_AUDIO_DIRECT_TEST','-DNPS64_HD_UI','-DNPS64_UI_VERSION="00.26"','-std=c11','-O2','-Wall','-Wextra','-Werror',
         *stack_check,*(['-mssse3'] if unit=='frontend' else []),*(['-Wno-unused-function'] if unit=='covers' else []),
         '-I'+str(ROOT/'src'),'-I'+str(core/'libretro-common/include'),
         '-I'+str(core/'mupen64plus-core/src/api'),
         '-I'+str(core/'mupen64plus-video-paraLLEl/parallel-rdp/vulkan-headers/include'),
         '-c',ROOT/('src/'+unit+'.c'),'-o',obj]);objects.append(obj)
for unit,extra in [('ps4_malloc',['-DNPS64_GPU','-DNPS64_GPU_GAME_TEST']),('ps4_dlmalloc',[])]:
    obj=build/(unit+'.o');run([build/'cc',*extra,'-O2','-c',ROOT/('src/third_party/'+unit+'.c'),'-o',obj]);objects.append(obj)
run([work/'llvm/usr/bin/ld.lld-18','-m','elf_x86_64','-pie','-z','max-page-size=0x4000','--script',
     ROOT/'scripts/ps4-gpu-link.x','--eh-frame-hdr','--wrap=pthread_once','--wrap=mprotect',
     '-L'+str(sdk/'lib'),sdk/'lib/crt1.o',*objects,'--start-group',core/'parallel_n64.a',
     '--whole-archive',*libraries,'--no-whole-archive','-lc++','-lc++abi','-lunwind','-lc','-lm','-lkernel',
     '-lSceLibcInternal','-lSceGnmDriver','-lSceVideoOut','-lScePad','-lSceUserService','-lSceAudioOut',
     '-lSceNet','-lSceSsl','-lSceHttp','-lSceSystemService','-lSceSysmodule','--end-group','-o',build/'nps64.elf'])
run(['python3',ROOT/'scripts/package_gpu_game_test.py','--reference-work',work,'--sdk',sdk])
