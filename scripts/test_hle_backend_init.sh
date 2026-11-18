#!/usr/bin/env bash
set -euo pipefail
cd /work
core=${1:-build/native-gpu-asan}
out=build/hle-backend-init-test
mkdir -p "$out"
args=(-resource-dir=/work/build/tools/usr/lib/llvm-18/lib/clang/18 -DGRANITE_VULKAN_MT -std=c++17 -O1 -g -fsanitize=address,undefined -ffunction-sections -fdata-sections
 -I"$core/mupen64plus-video-paraLLEl" -I"$core/mupen64plus-video-angrylion"
 -I"$core/mupen64plus-core/src/api" -I"$core/libretro-common/include"
 -I"$core/mupen64plus-video-paraLLEl/parallel-rdp/parallel-rdp"
 -I"$core/mupen64plus-video-paraLLEl/parallel-rdp/volk"
 -I"$core/mupen64plus-video-paraLLEl/parallel-rdp/vulkan"
 -I"$core/mupen64plus-video-paraLLEl/parallel-rdp/util"
 -I"$core/mupen64plus-video-paraLLEl/parallel-rdp/vulkan-headers/include")
for unit in parallel parallel_hle; do
 clang++-21 "${args[@]}" -c "$core/mupen64plus-video-paraLLEl/$unit.cpp" -o "$out/$unit.o"
done
clang++-21 "${args[@]}" tests/test_hle_backend_init.cpp "$out/parallel.o" "$out/parallel_hle.o" \
 -fuse-ld=lld -Wl,--gc-sections -o "$out/test"
"$out/test"
