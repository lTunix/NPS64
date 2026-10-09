#!/usr/bin/env bash
set -euo pipefail
cd /work
sdk=/work/toolchain/OpenOrbis/PS4Toolchain
sysroot=/work/build/gpu/sysroot
mkdir -p build/hle-probe build/stage/sce_sys out
python3 scripts/compile_hle_probe.py
for unit in hle_probe_frontend gpu_hle_probe hle_display_list gpu_boot platform_ps4 runtime_ps4 log; do
    scripts/ps4-gpu-cc -DNPS64_GPU -DNPS64_HLE_PROBE -std=c11 -O2 -Wall -Wextra -Werror -Isrc -Ivendor/parallel-n64/mupen64plus-video-paraLLEl/parallel-rdp/vulkan-headers/include -c "src/$unit.c" -o "build/hle-probe/$unit.o"
done
scripts/ps4-gpu-cc -DNPS64_GPU -O2 -c src/third_party/ps4_malloc.c -o build/hle-probe/malloc.o
scripts/ps4-gpu-cc -O2 -c src/third_party/ps4_dlmalloc.c -o build/hle-probe/dlmalloc.o
mesa=build/gpu/mesa-build
libs=(src/amd/vulkan/libvulkan_radeon.a src/amd/addrlib/libaddrlib.a
    src/amd/common/libamd_common.a src/amd/compiler/libaco.a src/c11/impl/libmesa_util_c11.a
    src/compiler/libcompiler.a src/compiler/nir/libnir.a src/compiler/spirv/libvtn.a
    src/util/blake3/libblake3.a src/util/libmesa_util.a src/util/libmesa_util_clflush.a
    src/util/libmesa_util_clflushopt.a src/util/libmesa_util_simd.a src/util/libparson.a
    src/util/libxmlconfig.a src/vulkan/util/libvulkan_util.a)
archives=();for lib in "${libs[@]}"; do archives+=("$mesa/$lib");done
ld.lld-21 -m elf_x86_64 -pie -z max-page-size=0x4000 --script scripts/ps4-gpu-link.x --eh-frame-hdr --wrap=pthread_once --wrap=mprotect -L"$sdk/lib" "$sdk/lib/crt1.o" build/hle-probe/{hle_probe_frontend,gpu_hle_probe,hle_display_list,gpu_boot,platform_ps4,runtime_ps4,log,malloc,dlmalloc}.o --start-group --whole-archive "${archives[@]}" --no-whole-archive "$sysroot/lib/libc++.a" "$sysroot/lib/libc++abi.a" "$sdk/lib/libunwind.a" -lc -lm -lkernel -lSceLibcInternal -lSceVideoOut -lScePad -lSceUserService -lSceAudioOut -lSceSystemService -lSceSysmodule --end-group -o build/nps64.elf
"$sdk/bin/linux/create-fself" -in=build/nps64.elf -out=build/nps64.oelf --eboot build/stage/eboot.bin --paid 0x3100000000000002
python3 scripts/check_elf.py
python3 scripts/check_gpu_boot.py --hle-probe
python3 scripts/assets.py
NPS64_VERSION=00.23 bash scripts/package.sh
bash scripts/verify_pkg.sh
cp out/IV0000-NPSX00064_00-NPS64PS400000000.pkg out/NPS64-00.23-hle-graphics-probe.pkg
python3 - <<'PY'
import hashlib
from pathlib import Path
p=Path('out/NPS64-00.23-hle-graphics-probe.pkg')
Path('out/NPS64-00.23-hle-graphics-probe-SHA256.txt').write_text(hashlib.sha256(p.read_bytes()).hexdigest()+'  '+p.name+'\n')
assert hashlib.sha256(Path('out/NPS64-00.13.pkg').read_bytes()).hexdigest()=='83412904ab8e66d13fea71aadc7ae224c860e3272c23a6e3cfeeb5b3ecf39c22'
elf=Path('build/nps64.elf').read_bytes()
assert b'NPS64 00.23 HLE GRAPHICS PROBE' in elf
assert b'PASS 64 graphics submissions' in elf
assert b'EmuThread: M64CMD_EXECUTE' not in elf
print('PASS probe version, graphics test payload, no N64 core, stable package unchanged')
PY
