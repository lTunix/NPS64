#!/usr/bin/env bash
# Rebuild the CPU rasterizer profile locally; ROM is supplied by the user.
set -euo pipefail
cd /work
rom="${1:?Pass the local Super Mario 64 PAL ROM path; optionally Zelda PAL as argument 2}"
test -f "$rom"
training=$(mktemp -d /work/build/cpu-rdp-training.XXXXXX)
core=${NPS64_TRAINING_CORE:-build/native-rdp-training}
NATIVE_JIT_DIR="$core" NPS64_RDP_PROFILE_FLAGS=-fprofile-instr-generate bash scripts/build_native_jit.sh
BENCH_JIT=1 BENCH_CORE="$core" BENCH_WORKERS=1 BENCH_CPU=jit BENCH_SM64=1 \
BENCH_FRAMES=4000 BENCH_TIMEOUT=300 BENCH_RDP_PROFILE=1 \
LLVM_PROFILE_FILE="$training/game-%m-%p.profraw" \
bash scripts/bench.sh "$rom" > "$training/game.txt"
if [ "$#" -ge 2 ]; then
  test -f "$2"
  BENCH_JIT=1 BENCH_CORE="$core" BENCH_WORKERS=1 BENCH_CPU=jit \
  BENCH_FRAMES=3500 BENCH_TIMEOUT=300 BENCH_RDP_PROFILE=1 \
  LLVM_PROFILE_FILE="$training/zelda-%m-%p.profraw" \
  bash scripts/bench.sh "$2" > "$training/zelda.txt"
fi
clang -fprofile-instr-generate -O2 -I"$core/mupen64plus-video-angrylion" \
  -I"$core/libretro-common/include" "$core/tools/angrylion_verify.c" \
  "$core/mupen64plus-video-angrylion/n64video.o" \
  "$core/mupen64plus-video-angrylion/parallel_al.o" \
  "$core/libretro-common/rthreads/rthreads.o" \
  "$core/libretro-common/rthreads/retro_eventcount.o" \
  "$core/libretro-common/features/features_cpu.o" -lpthread -lm -o "$core/angrylion_verify"
for mode in 0 1 2; do
  LLVM_PROFILE_FILE="$training/fixture-%m-%p.profraw" \
    "$core/angrylion_verify" 1 400 4 "$mode" 2 2 > "$training/mode-$mode.txt"
done
output=${NPS64_PROFILE_OUTPUT:-assets/cpu-rdp-sm64.profdata}
llvm-profdata merge -o "$output" "$training/"*.profraw
llvm-profdata show "$output"
sha256sum "$output"
echo "Profile regenerated. Validate pixel hashes and performance before packaging."
