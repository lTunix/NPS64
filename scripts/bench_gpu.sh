#!/usr/bin/env bash
set -euo pipefail
cd /work
rom="${1:?Local ROM path}"
mkdir -p build/gpu-bench/games build/gpu-bench/saves build/gpu-bench/system
ln -sf "$(realpath "$rom")" build/gpu-bench/games/benchmark.z64
clang-21 -std=c11 -D_GNU_SOURCE -D_POSIX_C_SOURCE=200809L -DNPS64_GPU -DNPS64_TEST_PS4_GPU_PROFILE -DDYNAREC -DNEW_DYNAREC=2 -D_M_X64 -DARCH_MIN_SSE2 -D__LIBRETRO__ -DDATA_ROOT='"/work/build/gpu-bench"' -O2 -Wall -Wextra -Werror -Isrc -Ivendor/parallel-n64/libretro-common/include -Ivendor/parallel-n64/mupen64plus-core/src -Ivendor/parallel-n64/mupen64plus-core/src/api -Ivendor/parallel-n64/include src/frontend.c src/gpu_vulkan.c src/storage.c src/log.c tests/platform_bench.c build/native-gpu/parallel_n64.a -lstdc++ -ldl -lm -lpthread -o build/bench-gpu
: > build/gpu-bench/nps64.log
VK_ICD_FILENAMES=/usr/share/vulkan/icd.d/lvp_icd.x86_64.json timeout "${BENCH_TIMEOUT:-300}" build/bench-gpu > build/gpu-bench-results.log 2>&1
cat build/gpu-bench-results.log
