#!/usr/bin/env bash
set -euo pipefail
cd /work
clang -std=c11 -D_DEFAULT_SOURCE -D_POSIX_C_SOURCE=200809L -D__ORBIS__ -DNPS64_GPU -DNPS64_GPU_GAME_TEST -DNPS64_GPU_PRESENT_TEST -DNPS64_TEST_TSC -DFONT_PATH='"assets/font.bin"' -g -fsanitize=address,undefined -Wall -Wextra -Werror -Wno-unused-parameter -Isrc -idirafter toolchain/OpenOrbis/PS4Toolchain/include tests/test_ps4_backend.c src/platform_ps4.c src/log.c -lm -lpthread -o build/present-72/test-scanout
ASAN_OPTIONS=detect_leaks=0 build/present-72/test-scanout
ASAN_OPTIONS=detect_leaks=0 TEST_GPU_REGISTER_DENIED=1 build/present-72/test-scanout
