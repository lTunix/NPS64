#!/usr/bin/env bash
set -euo pipefail
cd /work
sdk=/work/toolchain/OpenOrbis/PS4Toolchain
sysroot=/work/build/gpu/sysroot
dir=build/hle-game
test -f build/core-gpu/parallel_n64.a || GPU_CORE_ONLY=1 bash scripts/build_gpu.sh
mkdir -p "$dir/core/mupen64plus-video-angrylion" build/stage/sce_sys out
cp vendor/parallel-n64/mupen64plus-video-angrylion/rdp_emit_bridge.c "$dir/core/mupen64plus-video-angrylion/"
python3 scripts/patch_hle_geometry.py "$dir/core"
scripts/ps4-gpu-cc -std=c99 -O2 -Ivendor/parallel-n64/mupen64plus-video-angrylion -c "$dir/core/mupen64plus-video-angrylion/rdp_emit_bridge.c" -o "$dir/rdp_emit_bridge.o"
scripts/ps4-gpu-cc -std=c99 -O2 -fno-strict-aliasing -Ivendor/parallel-n64/mupen64plus-core/src -Ivendor/parallel-n64/libretro-common/include -Ivendor/parallel-n64/mupen64plus-video-angrylion -Ivendor/parallel-n64/mupen64plus-video-angrylion/n64video -c "$dir/core/mupen64plus-video-angrylion/n64video.c" -o "$dir/n64video.o"
cp build/core-gpu/parallel_n64.a "$dir/core.a"
llvm-ar-21 r "$dir/core.a" "$dir/rdp_emit_bridge.o" "$dir/n64video.o"
python3 scripts/compile_hle_probe.py
includes=(-Isrc -Ivendor/parallel-n64/libretro-common/include -Ivendor/parallel-n64/mupen64plus-video-angrylion -Ivendor/parallel-n64/mupen64plus-video-paraLLEl/parallel-rdp/vulkan-headers/include)
for unit in frontend gpu_hle_probe hle_display_list hle_game_geometry hle_game_material gpu_boot platform_ps4 storage runtime_ps4 log; do
    flags=(-DNPS64_GPU);if [ "$unit" = frontend ];then flags=();fi
    scripts/ps4-gpu-cc "${flags[@]}" -DNPS64_HLE_GAME -std=c11 -O2 -Wall -Wextra -Werror "${includes[@]}" -c "src/$unit.c" -o "$dir/$unit.o"
done
scripts/ps4-gpu-cc -DNPS64_GPU -O2 -c src/third_party/ps4_malloc.c -o "$dir/malloc.o"
scripts/ps4-gpu-cc -O2 -c src/third_party/ps4_dlmalloc.c -o "$dir/dlmalloc.o"
mesa=build/gpu/mesa-build
libs=(src/amd/vulkan/libvulkan_radeon.a src/amd/addrlib/libaddrlib.a src/amd/common/libamd_common.a src/amd/compiler/libaco.a src/c11/impl/libmesa_util_c11.a src/compiler/libcompiler.a src/compiler/nir/libnir.a src/compiler/spirv/libvtn.a src/util/blake3/libblake3.a src/util/libmesa_util.a src/util/libmesa_util_clflush.a src/util/libmesa_util_clflushopt.a src/util/libmesa_util_simd.a src/util/libparson.a src/util/libxmlconfig.a src/vulkan/util/libvulkan_util.a)
archives=();for lib in "${libs[@]}";do archives+=("$mesa/$lib");done
ld.lld-21 -m elf_x86_64 -pie -z max-page-size=0x4000 --script scripts/ps4-gpu-link.x --eh-frame-hdr --wrap=pthread_once --wrap=mprotect -L"$sdk/lib" "$sdk/lib/crt1.o" "$dir"/{frontend,gpu_hle_probe,hle_display_list,hle_game_geometry,hle_game_material,gpu_boot,platform_ps4,storage,runtime_ps4,log,malloc,dlmalloc}.o --start-group "$dir/core.a" --whole-archive "${archives[@]}" --no-whole-archive "$sysroot/lib/libc++.a" "$sysroot/lib/libc++abi.a" "$sdk/lib/libunwind.a" -lc -lm -lkernel -lSceLibcInternal -lSceVideoOut -lScePad -lSceUserService -lSceAudioOut -lSceSystemService -lSceSysmodule --end-group -o build/nps64.elf
"$sdk/bin/linux/create-fself" -in=build/nps64.elf -out=build/nps64.oelf --eboot build/stage/eboot.bin --paid 0x3100000000000002
python3 scripts/check_elf.py
python3 scripts/check_gpu_boot.py
python3 scripts/assets.py
NPS64_VERSION=00.66 bash scripts/package.sh
bash scripts/verify_pkg.sh
cp out/IV0000-NPSX00064_00-NPS64PS400000000.pkg out/NPS64-00.66-hle-vulkan-fallback.pkg
sha256sum out/NPS64-00.66-hle-vulkan-fallback.pkg > out/NPS64-00.66-hle-vulkan-fallback-SHA256.txt
python3 - <<'PY'
import hashlib
from pathlib import Path
elf=Path('build/nps64.elf').read_bytes()
assert b'NPS64 00.66 Ari64 / Angrylion + Vulkan game geometry sequence' in elf
assert b'%s Vulkan sequence frames=' in elf
assert b'scene_frames=%u uniform_frames=%u' in elf
print('PASS HLE Vulkan fallback payload')
PY
