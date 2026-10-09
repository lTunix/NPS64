#!/usr/bin/env bash
# Train a candidate profile; keep the released profile immutable until validated.
set -euo pipefail
cd /work
golden=${1:?Pass the local GoldenEye USA ROM}
mario=${2:?Pass the local Super Mario 64 PAL ROM}
zelda=${3:?Pass the local Ocarina of Time PAL GC ROM}
for rom in "$golden" "$mario" "$zelda"; do test -f "$rom"; done
mkdir -p build
training=$(mktemp -d /work/build/cpu-games-training.XXXXXX)
core=${NPS64_TRAINING_CORE:-build/native-games-training}
NATIVE_JIT_DIR="$core" NPS64_RDP_PROFILE_FLAGS=-fprofile-instr-generate bash scripts/build_native_jit.sh
for title in golden mario zelda; do
 case "$title" in
 golden) rom="$golden"; frames=8000; controls=BENCH_GOLDENEYE=1 ;;
 mario) rom="$mario"; frames=2000; controls=BENCH_SM64=1 ;;
 zelda) rom="$zelda"; frames=2300; controls=BENCH_ZELDA_MENU=1 ;;
 esac
 env "$controls" BENCH_RDP_PROFILE=1 LLVM_PROFILE_FILE="$training/$title-%m-%p.profraw" \
 BENCH_JIT=1 BENCH_CPU=jit BENCH_CORE="$core" BENCH_WORKERS=1 \
 BENCH_FRAMES="$frames" BENCH_TIMEOUT=900 bash scripts/bench.sh "$rom" > "$training/$title.txt"
done
output=${NPS64_PROFILE_OUTPUT:-build/cpu-rdp-games-candidate.profdata}
llvm-profdata merge "$training/"*.profraw -o "$output"
llvm-profdata show "$output"
sha256sum "$output"
echo 'Validate full images and sequential performance comparisons before using this profile in a PKG.'
