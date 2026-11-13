#!/usr/bin/env bash
set -euo pipefail
cd /work
core=${1:-build/native-core}
clang -std=c11 -D__LIBRETRO__ -O1 -g -fsanitize=address,undefined -Wall -Wextra -Werror \
  -I"$core/libretro-common/include" -I"$core/mupen64plus-core/src" \
  -I"$core/mupen64plus-core/subprojects/md5" \
  -I"$core/include" \
  -I"$core/mupen64plus-core/custom" \
  tests/test_audio_field.c \
  "$core/mupen64plus-core/src/plugin/audio_libretro/audio_backend_libretro.c" \
  -o build/test-audio-field
build/test-audio-field
