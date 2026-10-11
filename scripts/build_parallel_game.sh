#!/usr/bin/env bash
set -euo pipefail
cd /work
sdk=/work/toolchain/OpenOrbis/PS4Toolchain
sysroot=/work/build/gpu/sysroot
game=/work/build/parallel-game
export NPS64_STAGE_DIR="$game/stage" NPS64_ELF_DIR="$game"
export NPS64_CONTENT_ID=IV0000-NPSP00062_00-NPS64PARALLEL000
export NPS64_TITLE_ID=NPSP00062 NPS64_TITLE='NPS64 - ParaLLEl-RDP experimental'
export NPS64_VERSION=00.62 NPS64_VERIFY_DIR="$game/pkg-verify"
export NPS64_PACKAGE_PATH="out/$NPS64_CONTENT_ID.pkg"
mkdir -p build/parallel-game/core "$NPS64_STAGE_DIR/sce_sys" "$game/objects" out
chmod +x scripts/ps4-gpu-cc scripts/ps4-gpu-cxx
if [ "${GPU_LINK_ONLY:-0}" != 1 ]; then
    python3 scripts/compile_parallel_game_binning.py
    tar -C vendor/parallel-n64 --exclude=.git -cf - . | tar -C build/parallel-game/core -xf -
    python3 scripts/patch_core.py build/parallel-game/core
    python3 scripts/patch_parallel_game_binning.py build/parallel-game/core
    args=(platform=unix ARCH=x86_64 WITH_DYNAREC=x86_64 WANT_HACKTARUX=0 HAVE_ARI64_X64=1 HAVE_OPENGL=0 HAVE_PARALLEL=1 HAVE_PARALLEL_RSP=0 HAVE_THR_AL=1 STATIC_LINKING=1 TARGET=parallel_n64.a "CPUOPTS=-O2 -DNDEBUG" GIT_VERSION=890323d CC=/work/scripts/ps4-gpu-cc CXX=/work/scripts/ps4-gpu-cxx AR=llvm-ar-21)
    make -C build/parallel-game/core asm-defines-regen "${args[@]}" > build/parallel-game/core-build.log 2>&1
    make -C build/parallel-game/core -j"${JOBS:-4}" "${args[@]}" >> build/parallel-game/core-build.log 2>&1 || { tail -70 build/parallel-game/core-build.log; exit 1; }
fi
if [ "${GPU_CORE_ONLY:-0}" = 1 ]; then exit 0; fi
includes=(-Isrc -Ivendor/parallel-n64/libretro-common/include -Ivendor/parallel-n64/mupen64plus-core/src/api -Ivendor/parallel-n64/mupen64plus-video-paraLLEl/parallel-rdp/vulkan-headers/include)
experimental=(-DNPS64_GPU_EXPERIMENTAL -DNPS64_PARALLEL_GAME
  '-DDATA_ROOT="/data/NPS64-Parallel"' '-DNPS64_GAMES_ROOT="/data/NPS64/games"')
for unit in frontend gpu_vulkan gpu_boot platform_ps4 storage runtime_ps4 log; do
    scripts/ps4-gpu-cc -DNPS64_GPU "${experimental[@]}" -std=c11 -O2 -Wall -Wextra "${includes[@]}" -c "src/$unit.c" -o "$game/objects/$unit.o"
done
scripts/ps4-gpu-cc -DNPS64_GPU -O2 -c src/third_party/ps4_malloc.c -o "$game/objects/malloc.o"
scripts/ps4-gpu-cc -O2 -c src/third_party/ps4_dlmalloc.c -o "$game/objects/dlmalloc.o"
mesa=build/gpu/mesa-build
libs=(src/amd/vulkan/libvulkan_radeon.a src/amd/addrlib/libaddrlib.a
    src/amd/common/libamd_common.a src/amd/compiler/libaco.a src/c11/impl/libmesa_util_c11.a
    src/compiler/libcompiler.a src/compiler/nir/libnir.a src/compiler/spirv/libvtn.a
    src/util/blake3/libblake3.a src/util/libmesa_util.a src/util/libmesa_util_clflush.a
    src/util/libmesa_util_clflushopt.a src/util/libmesa_util_simd.a src/util/libparson.a
    src/util/libxmlconfig.a src/vulkan/util/libvulkan_util.a)
archives=();for lib in "${libs[@]}"; do archives+=("$mesa/$lib");done
ld.lld-21 -m elf_x86_64 -pie -z max-page-size=0x4000 --script scripts/ps4-gpu-link.x --eh-frame-hdr --wrap=pthread_once --wrap=mprotect -L"$sdk/lib" "$sdk/lib/crt1.o" "$game"/objects/{frontend,gpu_vulkan,gpu_boot,platform_ps4,storage,runtime_ps4,log,malloc,dlmalloc}.o --start-group build/parallel-game/core/parallel_n64.a --whole-archive "${archives[@]}" --no-whole-archive "$sysroot/lib/libc++.a" "$sysroot/lib/libc++abi.a" "$sdk/lib/libunwind.a" -lc -lm -lkernel -lSceLibcInternal -lSceVideoOut -lScePad -lSceUserService -lSceAudioOut -lSceSystemService -lSceSysmodule --end-group -o "$game/nps64.elf"
"$sdk/bin/linux/create-fself" -in="$game/nps64.elf" -out="$game/nps64.oelf" --eboot "$NPS64_STAGE_DIR/eboot.bin" --paid 0x3100000000000002 --authinfo 000000000000000000000000001C004000FF000000000080000000000000000000000000000000000000008000400040000000000000008000000000000000080040FFFF000000F000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
python3 scripts/check_elf.py
python3 scripts/check_gpu_boot.py
python3 scripts/assets.py
bash scripts/package.sh
bash scripts/verify_pkg.sh
cp "$NPS64_PACKAGE_PATH" out/NPS64-00.62-parallel-rdp-test.pkg
sha256sum out/NPS64-00.62-parallel-rdp-test.pkg > out/NPS64-00.62-parallel-rdp-test-SHA256.txt
python3 scripts/check_parallel_game.py
