#!/usr/bin/env bash
set -euo pipefail
cd /work
core=${1:-build/gpu-game-test/core}
worker="$core/mupen64plus-video-paraLLEl/parallel-rdp/parallel-rdp"
clang -std=c11 -D_GNU_SOURCE -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer -Wall -Wextra -Werror \
 -I"$worker" -Ivendor/parallel-n64/libretro-common/include tests/test_pipeline_demand.c \
 "$worker/rdp_worker.c" vendor/parallel-n64/libretro-common/rthreads/retro_eventcount.c \
 vendor/parallel-n64/libretro-common/rthreads/rthreads.c -lpthread -o build/test-pipeline-demand
timeout 20 build/test-pipeline-demand
