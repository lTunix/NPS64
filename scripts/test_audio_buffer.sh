#!/usr/bin/env bash
set -euo pipefail
cd /work
for profile in checkpoint large; do
 flags=()
 if [ "$profile" = large ]; then flags=(-DNPS64_AUDIO_BUFFER_TEST); fi
 clang "${flags[@]}" -std=c11 -O2 -g -fsanitize=address,undefined -Wall -Wextra -Werror -Isrc tests/test_audio_buffer.c -lm -o "build/test-audio-buffer-$profile"
 "build/test-audio-buffer-$profile"
done
