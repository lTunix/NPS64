#!/usr/bin/env bash
set -euo pipefail
cd /work
mkdir -p build
clang -std=c11 -O2 -g -fsanitize=address,undefined -Wall -Wextra -Werror \
  -Isrc tests/test_frame_pacing.c -lm -o build/test-frame-pacing
build/test-frame-pacing
