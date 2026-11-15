#!/usr/bin/env bash
set -euo pipefail
cd /work
mkdir -p build/present-72
clang -mssse3 -std=c11 -D_POSIX_C_SOURCE=200809L -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer -Wall -Wextra -Werror -Isrc -Ivendor/parallel-n64/mupen64plus-video-paraLLEl/parallel-rdp/vulkan-headers/include tests/test_gpu_presenter.c src/gpu_presenter.c src/log.c -l:libvulkan.so.1 -lpthread -o build/present-72/test-presenter
ASAN_OPTIONS=detect_leaks=0 VK_DRIVER_FILES=/usr/share/vulkan/icd.d/lvp_icd.json VK_LAYER_PATH=/work/build/tools/usr/share/vulkan/explicit_layer.d VK_INSTANCE_LAYERS=VK_LAYER_KHRONOS_validation build/present-72/test-presenter
