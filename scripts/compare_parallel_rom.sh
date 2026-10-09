#!/usr/bin/env bash
# Identical ROM, input, CPU and RSP; renderer is the only run-time difference.
set -euo pipefail
cd /work
rom="${1:?Local ROM path}"
tag="${2:-mario}"
[[ "$tag" =~ ^[A-Za-z0-9_-]+$ ]] || { echo "Invalid comparison tag"; exit 2; }
base="/work/build/parallel-comparison/$tag"
mkdir -p "$base"/data/{games,saves,system} "$base"/{cpu,gpu}
ln -sf "$(realpath "$rom")" "$base/data/games/benchmark.z64"
# Isolated empty save/config state for each pair; never access user saves.
printf 'NPS64DIAG5 6 0 1 1 1\n' > "$base/data/diagnostics.cfg"
clang-21 -std=c11 -D_GNU_SOURCE -D_POSIX_C_SOURCE=200809L -DNPS64_GPU \
  -DNPS64_TEST_PS4_GPU_PROFILE -DNPS64_PARALLEL_GAME -DNPS64_PARALLEL_CAPTURE \
  -DDYNAREC -DNEW_DYNAREC=2 -D_M_X64 -DARCH_MIN_SSE2 -D__LIBRETRO__ \
  -DDATA_ROOT="\"$base/data\"" -O2 -Wall -Wextra -Werror \
  -Isrc -Ivendor/parallel-n64/libretro-common/include -Ivendor/parallel-n64/mupen64plus-core/src \
  -Ivendor/parallel-n64/mupen64plus-core/src/api -Ivendor/parallel-n64/include \
  src/frontend.c src/gpu_vulkan.c src/storage.c src/log.c tests/platform_bench.c \
  "${PARALLEL_NATIVE_ARCHIVE:-build/parallel-game/native/parallel_n64.a}" -lstdc++ -ldl -lm -lpthread -o "$base/compare-rom"
export VK_ICD_FILENAMES=/usr/share/vulkan/icd.d/lvp_icd.x86_64.json
export BENCH_CPU=jit BENCH_RSP=cxd4
export BENCH_FRAMES="${BENCH_FRAMES:-1230}"
export NPS64_PARALLEL_CAPTURE_FRAMES="${NPS64_PARALLEL_CAPTURE_FRAMES:-150,300,480,600,900,1200}"
export NPS64_RDP_DIRECT_BINNING=0 NPS64_RDP_COLOR_ONLY=0
renderers=(cpu gpu)
if [ "${PARALLEL_CPU_ONLY:-0}" = 1 ]; then renderers=(cpu); fi
if [ "${PARALLEL_STREAM_ONLY:-0}" = 1 ]; then renderers=(gpu); export PARALLEL_RDP_DUMP_PATH="$base/commands.rdp"; fi
for renderer in "${renderers[@]}"; do
  # Fresh memory/save state in a distinct process for each renderer.
  : > "$base/data/nps64.log"
  find "$base/data/saves" -maxdepth 1 -type f -delete
  export BENCH_GPU=off
  if [ "$renderer" = gpu ]; then export BENCH_GPU=on; fi
  export NPS64_PARALLEL_CAPTURE_DIR="$base/$renderer"
  timeout "${BENCH_TIMEOUT:-600}" "$base/compare-rom" > "$base/$renderer/run.log" 2>&1
  cp "$base/data/nps64.log" "$base/$renderer/core.log"
  grep 'CAPTURE ' "$base/$renderer/run.log"
done
if [ "${PARALLEL_STREAM_ONLY:-0}" != 1 ] && [ "${PARALLEL_CPU_ONLY:-0}" != 1 ]; then python3 scripts/compare_parallel_images.py "$base"; fi
