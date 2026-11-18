#!/usr/bin/env bash
set -euo pipefail
cd /work
core=${1:-build/native-gpu-asan}
objects=()
for object in "$core"/mupen64plus-video-angrylion/rdp_emit*.o; do
    [[ "$object" == */rdp_emit_hle.o ]] || objects+=("$object")
done
clang-21 -std=c11 -O1 -g -fsanitize=address,undefined -Wall -Wextra -Werror \
    -I"$core/mupen64plus-video-angrylion" tests/test_gbi1_persp.c "${objects[@]}" \
    "$core/libretro-common/encodings/encoding_crc32.o" \
    "$core/libretro-common/features/features_cpu.o" -lm -o build/test-gbi1-persp
build/test-gbi1-persp
