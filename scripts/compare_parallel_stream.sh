#!/usr/bin/env bash
set -euo pipefail
cd /work
rom="${1:?Local ROM path}"; tag="${2:-mario-stream}"
export PARALLEL_STREAM_ONLY=1 BENCH_FRAMES="${BENCH_FRAMES:-650}"
export NPS64_PARALLEL_CAPTURE_FRAMES="${NPS64_PARALLEL_CAPTURE_FRAMES:-150,300,480,600}"
bash scripts/compare_parallel_rom.sh "$rom" "$tag"
base="/work/build/parallel-comparison/$tag"
clang-21 -O2 -Wall -Wextra -Werror -Ivendor/parallel-n64/mupen64plus-video-angrylion \
  tests/parallel_dump_cpu.c "${PARALLEL_NATIVE_ARCHIVE:-build/parallel-game/native/parallel_n64.a}" \
  -lpthread -lm -o "$base/replay-cpu"
"$base/replay-cpu" "$base/commands.rdp" "$base/cpu" "$NPS64_PARALLEL_CAPTURE_FRAMES" > "$base/cpu/run.log" 2>&1
python3 scripts/compare_parallel_images.py "$base" --same-stream
sha256sum "$base/commands.rdp" > "$base/commands-SHA256.txt"
