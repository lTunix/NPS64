#!/usr/bin/env bash
set -euo pipefail
cd /work
mkdir -p build/overlay-34
clang -std=c11 -O1 -g -fsanitize=address,undefined -Wall -Wextra -Werror -Isrc tests/test_overlay_cache.c -o build/overlay-34/test-cache
build/overlay-34/test-cache
NPS64_FRONTEND_TEST_FLAGS='-DNPS64_GPU_PRESENT_TEST -DNPS64_UPDATES' bash scripts/test_frontend_settings.sh
bash scripts/test_gpu_scanout.sh
