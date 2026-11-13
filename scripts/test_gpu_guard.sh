#!/usr/bin/env bash
set -euo pipefail
cd /work
compiler="${CC:-clang}"
common=(-std=c11 -Wall -Wextra -Werror -Isrc
  -Ivendor/parallel-n64/mupen64plus-video-paraLLEl/parallel-rdp/vulkan-headers/include)
units=(src/gpu_pipeline_guard.c src/gpu_compile_worker.c src/log.c)
"$compiler" "${common[@]}" -O1 -g -fsanitize=address,undefined -DNPS64_GUARD_ALLOC_TEST \
  tests/test_gpu_pipeline_guard.c "${units[@]}" -lpthread -Wl,--wrap=calloc -o build/test-gpu-guard-alloc
build/test-gpu-guard-alloc > build/gpu-guard-alloc.log 2>&1
NPS64_GPU_COMPILE_WORKER=1 build/test-gpu-guard-alloc > build/gpu-guard-alloc-worker.log 2>&1
# Real 64KiB thread stack; sanitizer stack expansion would alter this regression test.
"$compiler" "${common[@]}" -O2 -DNPS64_GUARD_SMALL_STACK_TEST \
  tests/test_gpu_pipeline_guard.c "${units[@]}" -lpthread -o build/test-gpu-guard-small-stack
build/test-gpu-guard-small-stack > build/guard-small-stack-new.log 2>&1
NPS64_GPU_COMPILE_WORKER=1 build/test-gpu-guard-small-stack > build/guard-small-stack-new-worker.log 2>&1
"$compiler" "${common[@]}" -O1 -g -fsanitize=address,undefined \
  tests/test_gpu_compile_worker.c src/gpu_compile_worker.c src/log.c -lpthread -o build/test-gpu-compile-worker
build/test-gpu-compile-worker
echo 'PASS: guard thresholds, allocation failures, 64KiB stack regression and worker handoff'
