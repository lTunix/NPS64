#!/usr/bin/env bash
# Train only new CPU49 spans, retaining all established CPU47 counters.
set -euo pipefail
cd /work
zelda=${1:?Pass the local Ocarina of Time PAL GC ROM path}
test -f "$zelda"
base=${NPS64_BASE_PROFILE:-assets/cpu-rdp-lanes.profdata}
test -f "$base"
training=$(mktemp -d /work/build/cpu-span-training.XXXXXX)
core=${NPS64_TRAINING_CORE:-build/native-span-training}
NATIVE_JIT_DIR="$core" NPS64_RDP_PROFILE_FLAGS=-fprofile-instr-generate bash scripts/build_native_jit.sh
BENCH_JIT=1 BENCH_CORE="$core" BENCH_WORKERS=1 BENCH_CPU=jit BENCH_ZELDA_MENU=1 \
  BENCH_FRAMES=3500 BENCH_TIMEOUT=300 BENCH_RDP_PROFILE=1 \
  LLVM_PROFILE_FILE="$training/zelda-%m-%p.profraw" \
  bash scripts/bench.sh "$zelda" > "$training/zelda.txt"
output=${NPS64_PROFILE_OUTPUT:-assets/cpu-rdp-sm64.profdata}
python3 scripts/merge_cpu_span_profile.py "$base" "$output" "$training/"*.profraw
sha256sum "$output"
echo 'Validate complete images and performance before packaging.'
