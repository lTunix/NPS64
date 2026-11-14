#!/usr/bin/env bash
set -euo pipefail
cd /work
voice=vendor/parallel-n64/mupen64plus-rsp-hle/src
clang -std=c11 -O1 -g -fsanitize=address,undefined -ffunction-sections -fdata-sections -Wall -Wextra -Werror -I"$voice" -Ivendor/parallel-n64/libretro-common/include tests/test_audio_voice_isolation.c "$voice/alist.c" "$voice/audio.c" "$voice/resample_hq.c" -Wl,--gc-sections -lm -o build/test-audio-voice-isolation
build/test-audio-voice-isolation
