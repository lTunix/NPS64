#!/usr/bin/env bash
set -euo pipefail
cd /work
mkdir -p build/widescreen-36
clang -std=c11 -D_POSIX_C_SOURCE=200809L -O1 -g -fsanitize=address,undefined -Wall -Wextra -Werror -Isrc tests/test_widescreen.c src/cheats.c src/storage.c -o build/widescreen-36/test-profiles
build/widescreen-36/test-profiles
python3 tests/test_widescreen_core.py
clang -std=gnu11 -O1 -g -fsanitize=address,undefined -Wall -Wextra -Werror -Isrc -Ibuild/gpu-game-test/core/mupen64plus-core/src -Ivendor/parallel-n64/libretro-common/include build/widescreen-36/test-core.c -o build/widescreen-36/test-core
build/widescreen-36/test-core
