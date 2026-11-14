#!/usr/bin/env bash
set -euo pipefail
cd /work
clang -std=c11 -D_GNU_SOURCE -DDATA_ROOT='"/work/build/save-cache-test"' -O1 -g -ffunction-sections -fdata-sections -fsanitize=address,undefined -Wall -Wextra -Werror -Isrc -Ivendor/parallel-n64/libretro-common/include tests/test_save_cache.c src/storage.c -Wl,--gc-sections -Wl,--wrap=file_write_atomic -lm -o build/test-save-cache
build/test-save-cache
