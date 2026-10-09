#!/usr/bin/env bash
# Compare complete color/depth buffers, including off-screen triangles and RTT.
set -euo pipefail
cd /work
before=${1:?Pass the baseline native core archive}
after=${2:?Pass the candidate native core archive}
mkdir -p build/cpu-rdp-check
for variant in before after; do
  archive=$before
  if [ "$variant" = after ]; then archive=$after; fi
  clang -O2 -Ivendor/parallel-n64/mupen64plus-video-angrylion \
    -Ivendor/parallel-n64/libretro-common/include \
    tests/angrylion_profile_verify.c "$archive" \
    -lpthread -lm -o "build/cpu-rdp-check/$variant"
  : > "build/cpu-rdp-check/$variant.txt"
  for profile in 0 1; do
  for workers in 1 4 6 8; do
    for mode in 0 1 2; do
      # High synchronization makes render-to-texture comparisons deterministic.
      NPS64_TEST_GAME_PROFILE="$profile" "build/cpu-rdp-check/$variant" "$workers" 120 4 "$mode" 2 2 \
        >> "build/cpu-rdp-check/$variant.txt"
    done
  done
  done
done
diff -u build/cpu-rdp-check/before.txt build/cpu-rdp-check/after.txt
echo 'PASS: 96 complete framebuffer/depth comparisons, both lane paths, workers 1/4/6/8'
