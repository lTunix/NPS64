#!/usr/bin/env bash
set -euo pipefail
cd /work
clang -std=c11 -D_GNU_SOURCE -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer -Wall -Wextra -Werror -Isrc tests/test_frame_capture.c src/frame_capture.c -lpthread -o build/test-frame-capture
build/test-frame-capture
