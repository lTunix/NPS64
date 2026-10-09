#!/usr/bin/env bash
set -euo pipefail
cd /work
mkdir -p build/gpu
aliases=(-Dmalloc=nps_malloc -Dfree=nps_free -Dcalloc=nps_calloc -Drealloc=nps_realloc -Dmemalign=nps_memalign -D__memalign=nps___memalign -Daligned_alloc=nps_aligned_alloc -Dposix_memalign=nps_posix_memalign -Dvalloc=nps_valloc -Dmalloc_usable_size=nps_usable_size)
clang -std=c11 -D_GNU_SOURCE -DNPS64_GPU -g -fsanitize=address,undefined "${aliases[@]}" -c src/third_party/ps4_malloc.c -o build/gpu/test-heap-api.o
clang -D_GNU_SOURCE -g -fsanitize=address,undefined -c src/third_party/ps4_dlmalloc.c -o build/gpu/test-dlmalloc.o
clang -D_GNU_SOURCE -g -fsanitize=address,undefined tests/test_gpu_heap.c build/gpu/test-heap-api.o build/gpu/test-dlmalloc.o -lpthread -o build/gpu/test-heap
build/gpu/test-heap
