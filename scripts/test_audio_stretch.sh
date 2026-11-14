#!/usr/bin/env bash
set -euo pipefail
cd /work
audio_flags=()
if [ "${NPS64_AUDIO_LARGE_BUFFER:-0}" = 1 ]; then audio_flags=(-DNPS64_AUDIO_BUFFER_TEST); fi
clang "${audio_flags[@]}" -std=c11 -O2 -g -fsanitize=address,undefined -Wall -Wextra -Werror -Isrc tests/test_audio_stretch.c -lm -o build/test-audio-stretch
build/test-audio-stretch
clang "${audio_flags[@]}" -std=c11 -O2 -g -fsanitize=address,undefined -Wall -Wextra -Werror -Isrc tests/test_audio_queue.c -o build/test-audio-queue
build/test-audio-queue
clang "${audio_flags[@]}" -std=c11 -O2 -g -fsanitize=address,undefined -Wall -Wextra -Werror -Isrc tests/test_audio_resampler.c -lm -o build/test-audio-resampler
build/test-audio-resampler
clang "${audio_flags[@]}" -std=c11 -D_DEFAULT_SOURCE -D_POSIX_C_SOURCE=200809L -D__ORBIS__ -DNPS64_GPU -DNPS64_TEST_TSC -DFONT_PATH='"assets/font.bin"' -g -fsanitize=address,undefined -Wall -Wextra -Werror -Wno-unused-parameter -Isrc -idirafter toolchain/OpenOrbis/PS4Toolchain/include tests/test_ps4_backend.c src/platform_ps4.c src/log.c -lm -lpthread -o build/test-ps4-backend-audio
TEST_AUDIO=1 build/test-ps4-backend-audio
TEST_AUDIO=1 TEST_AUDIO_PRIORITY_DENIED=1 build/test-ps4-backend-audio
