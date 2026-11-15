#!/usr/bin/env bash
set -euo pipefail
cd /work
flags=(-DNPS64_AUDIO_BUFFER_TEST -DNPS64_AUDIO_DIRECT_TEST -DNPS64_AUDIO_RECOVERY_TEST)
clang "${flags[@]}" -std=c11 -O2 -g -fsanitize=address,undefined -Wall -Wextra -Werror -Isrc tests/test_audio_recovery.c -o build/test-audio-recovery
build/test-audio-recovery
clang "${flags[@]}" -std=c11 -D_DEFAULT_SOURCE -D_POSIX_C_SOURCE=200809L -D__ORBIS__ -DNPS64_GPU -DNPS64_TEST_TSC -DFONT_PATH='"assets/font.bin"' -g -fsanitize=address,undefined -Wall -Wextra -Werror -Wno-unused-parameter -Isrc -idirafter toolchain/OpenOrbis/PS4Toolchain/include tests/test_ps4_backend.c src/platform_ps4.c src/log.c -lm -lpthread -o build/test-ps4-backend-recovery
TEST_AUDIO=1 build/test-ps4-backend-recovery
TEST_AUDIO=1 TEST_AUDIO_PRIORITY_DENIED=1 build/test-ps4-backend-recovery
