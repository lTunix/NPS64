#!/usr/bin/env bash
set -euo pipefail
cd /work
voice=vendor/parallel-n64/mupen64plus-rsp-hle/src
clang -std=c11 -O2 -g -fsanitize=address,undefined -Wall -Wextra -Werror -I"$voice" tests/test_audio_voice_quality.c "$voice/resample_hq.c" -lm -o build/test-audio-voice-quality
build/test-audio-voice-quality
