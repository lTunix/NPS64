#!/usr/bin/env bash
set -euo pipefail
cd /work
audio_flags=()
if [ "${NPS64_CAPTURE_HQ:-0}" = 1 ]; then audio_flags=(-DNPS64_AUDIO_HQ_TEST); fi
if [ "${NPS64_CAPTURE_CLEAN:-0}" = 1 ]; then audio_flags=(-DNPS64_AUDIO_HQ_TEST -DNPS64_AUDIO_CLEAN_TEST); fi
clang++-21 "${audio_flags[@]}" -resource-dir=/work/build/tools/usr/lib/llvm-18/lib/clang/18 -x c -std=c11 -D_GNU_SOURCE -D_POSIX_C_SOURCE=200809L -DNPS64_GPU -DNPS64_HOST_COMPILE_WORKER_TEST -DDATA_ROOT='"/work/build/audio-17-data"' -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer -Wall -Wextra -Werror -Isrc -Ivendor/parallel-n64/libretro-common/include -Ivendor/parallel-n64/mupen64plus-core/src -Ivendor/parallel-n64/mupen64plus-core/custom -Ivendor/parallel-n64/mupen64plus-core/src/api -Ivendor/parallel-n64/mupen64plus-video-paraLLEl/parallel-rdp/vulkan-headers/include src/frontend.c src/i18n.c src/settings.c src/gpu_vulkan.c src/gpu_api_trace.c src/gpu_compile_worker.c src/gpu_cache_store.c src/gpu_recipes.c src/gpu_crashlog.c src/sha256.c src/storage.c src/log.c tests/platform_audio_capture.c -x none build/native-gpu-asan/parallel_n64.a -Wl,--wrap=retro_set_environment -Wl,--wrap=push_audio_samples_via_libretro -Wl,--wrap=flush_audio_libretro -Wl,--wrap=ai_deliver_frame -fuse-ld=lld -ldl -lm -lpthread -o build/bench-audio-capture
