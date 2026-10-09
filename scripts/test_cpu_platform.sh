#!/usr/bin/env bash
set -euo pipefail
cd /work
mkdir -p build
clang -std=c11 -D_DEFAULT_SOURCE -D_POSIX_C_SOURCE=200809L -D__ORBIS__ -DNPS64_TEST_TSC -DFONT_PATH='"assets/font.bin"' -g -fsanitize=address,undefined -Wall -Wextra -Werror -Wno-unused-parameter -Isrc -idirafter toolchain/OpenOrbis/PS4Toolchain/include tests/test_ps4_backend.c src/platform_ps4.c src/log.c -lm -lpthread -o build/test-ps4-backend
build/test-ps4-backend
for scenario in TEST_TSC TEST_FLIP_JITTER TEST_AUDIO TEST_AUDIO_PRIORITY_DENIED TEST_CPU_PRIORITY_DENIED TEST_CPU_QUERY_DENIED TEST_CPU_HIGH_PRIORITY; do
  env "$scenario=1" build/test-ps4-backend
done
