#!/usr/bin/env bash
set -euo pipefail
cd /work
sdk=/work/toolchain/OpenOrbis/PS4Toolchain
sysroot=/work/build/gpu/sysroot
probe=/work/build/parallel-tile-probe
export NPS64_STAGE_DIR="$probe/stage" NPS64_ELF_DIR="$probe"
export NPS64_CONTENT_ID=IV0000-NPSP00054_00-NPS64PARALLEL000
export NPS64_TITLE_ID=NPSP00054 NPS64_TITLE='NPS64 - ParaLLEl tile capture'
export NPS64_VERSION=00.54 NPS64_VERIFY_DIR="$probe/pkg-verify"
export NPS64_PACKAGE_PATH="out/$NPS64_CONTENT_ID.pkg"
mkdir -p "$probe" "$NPS64_STAGE_DIR/sce_sys" out
test -f build/gpu/mesa-build/src/amd/vulkan/libvulkan_radeon.a || {
    echo 'Preparar GPU: bootstrap_gpu.sh y build_mesa_ps4.sh'; exit 1;
}
python3 scripts/prepare_parallel_tile_probe.py "${1:-/work/logs}"
for unit in parallel_probe_frontend parallel_tile_probe gpu_boot platform_ps4 runtime_ps4 log; do
    scripts/ps4-gpu-cc -DNPS64_GPU -DNPS64_PARALLEL_PROBE -DNPS64_PARALLEL_TILE_PROBE -std=c11 -O2 -Wall -Wextra -Werror \
      -Isrc -I"$probe" -Ivendor/parallel-n64/mupen64plus-video-paraLLEl/parallel-rdp/vulkan-headers/include \
      -c "src/$unit.c" -o "$probe/$unit.o"
done
scripts/ps4-gpu-cc -DNPS64_GPU -O2 -c src/third_party/ps4_malloc.c -o "$probe/malloc.o"
scripts/ps4-gpu-cc -O2 -c src/third_party/ps4_dlmalloc.c -o "$probe/dlmalloc.o"
mesa=build/gpu/mesa-build
libs=(src/amd/vulkan/libvulkan_radeon.a src/amd/addrlib/libaddrlib.a
    src/amd/common/libamd_common.a src/amd/compiler/libaco.a src/c11/impl/libmesa_util_c11.a
    src/compiler/libcompiler.a src/compiler/nir/libnir.a src/compiler/spirv/libvtn.a
    src/util/blake3/libblake3.a src/util/libmesa_util.a src/util/libmesa_util_clflush.a
    src/util/libmesa_util_clflushopt.a src/util/libmesa_util_simd.a src/util/libparson.a
    src/util/libxmlconfig.a src/vulkan/util/libvulkan_util.a)
archives=();for lib in "${libs[@]}"; do archives+=("$mesa/$lib");done
ld.lld-21 -m elf_x86_64 -pie -z max-page-size=0x4000 --script scripts/ps4-gpu-link.x \
  --eh-frame-hdr --wrap=pthread_once --wrap=mprotect -L"$sdk/lib" "$sdk/lib/crt1.o" \
  "$probe"/{parallel_probe_frontend,parallel_tile_probe,gpu_boot,platform_ps4,runtime_ps4,log,malloc,dlmalloc}.o \
  --start-group --whole-archive "${archives[@]}" --no-whole-archive \
  "$sysroot/lib/libc++.a" "$sysroot/lib/libc++abi.a" "$sdk/lib/libunwind.a" \
  -lc -lm -lkernel -lSceLibcInternal -lSceVideoOut -lScePad -lSceUserService -lSceAudioOut \
  -lSceSystemService -lSceSysmodule --end-group -o "$probe/nps64.elf"
"$sdk/bin/linux/create-fself" -in="$probe/nps64.elf" -out="$probe/nps64.oelf" \
  --eboot "$NPS64_STAGE_DIR/eboot.bin" --paid 0x3100000000000002
python3 scripts/check_elf.py
python3 scripts/check_gpu_boot.py --hle-probe
python3 scripts/assets.py
bash scripts/package.sh
bash scripts/verify_pkg.sh
cp "$NPS64_PACKAGE_PATH" out/NPS64-00.54-parallel-tile-probe.pkg
python3 scripts/check_parallel_tile_probe.py
