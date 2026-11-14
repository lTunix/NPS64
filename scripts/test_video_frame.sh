#!/usr/bin/env bash
set -euo pipefail
cd /work
clang -mssse3 -std=c11 -D_GNU_SOURCE -DNPS64_GPU -DNPS64_GPU_BORROW_TEST -DNPS64_IMAGE_SMOOTH_TEST -O1 -g -ffunction-sections -fdata-sections -fsanitize=address,undefined -Wall -Wextra -Werror -Isrc -Ivendor/parallel-n64/libretro-common/include tests/test_video_frame.c -Wl,--gc-sections -lm -o build/test-video-frame
build/test-video-frame
