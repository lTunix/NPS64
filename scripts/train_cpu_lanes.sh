#!/usr/bin/env bash
# Update changed rasterizer functions while retaining CPU46's other profiles.
set -euo pipefail
cd /work
mario=${1:?Pass the local Super Mario 64 PAL ROM path}
zelda=${2:?Pass the local Ocarina of Time PAL GC ROM path}
test -f "$mario"; test -f "$zelda"
base=${NPS64_BASE_PROFILE:-assets/cpu-rdp-base.profdata}
test -f "$base"
training=$(mktemp -d /work/build/cpu-lane-training.XXXXXX)
core=${NPS64_TRAINING_CORE:-build/native-lane-training}
NATIVE_JIT_DIR="$core" NPS64_RDP_PROFILE_FLAGS=-fprofile-instr-generate bash scripts/build_native_jit.sh
BENCH_JIT=1 BENCH_CORE="$core" BENCH_WORKERS=1 BENCH_CPU=jit BENCH_SM64=1 \
  BENCH_FRAMES=1500 BENCH_TIMEOUT=300 BENCH_RDP_PROFILE=1 \
  LLVM_PROFILE_FILE="$training/mario-%m-%p.profraw" \
  bash scripts/bench.sh "$mario" > "$training/mario.txt"
BENCH_JIT=1 BENCH_CORE="$core" BENCH_WORKERS=1 BENCH_CPU=jit \
  BENCH_FRAMES=3500 BENCH_TIMEOUT=300 BENCH_RDP_PROFILE=1 \
  LLVM_PROFILE_FILE="$training/zelda-%m-%p.profraw" \
  bash scripts/bench.sh "$zelda" > "$training/zelda.txt"
output=${NPS64_PROFILE_OUTPUT:-assets/cpu-rdp-sm64.profdata}
python3 scripts/merge_cpu_lane_profile.py "$base" "$output" "$training/"*.profraw
sha256sum "$output"
echo 'Profile regenerated; validate complete images and performance before packaging.'
