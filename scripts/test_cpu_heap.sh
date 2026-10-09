#!/usr/bin/env bash
set -euo pipefail
clang -std=c11 -D_DEFAULT_SOURCE -O1 -g -fsanitize=address,undefined -Dmalloc=nps_malloc -Dfree=nps_free -Dcalloc=nps_calloc -Drealloc=nps_realloc -Dposix_memalign=nps_posix_memalign -Daligned_alloc=nps_aligned_alloc -Dmemalign=nps_memalign -Dmalloc_usable_size=nps_malloc_usable_size -c src/third_party/ps4_malloc.c -o build/cpu43-heap-test.o
clang -std=c11 -O1 -g -fsanitize=address,undefined -Wall -Wextra -Werror tests/test_cpu_heap.c build/cpu43-heap-test.o -o build/test-cpu-heap
for mode in full allocation-fail map-fail limited; do build/test-cpu-heap "$mode"; done
