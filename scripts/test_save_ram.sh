#!/usr/bin/env bash
set -euo pipefail
cd /work
clang -std=c11 -D_POSIX_C_SOURCE=200809L -O1 -g -fsanitize=address,undefined -Wall -Wextra -Werror -Isrc tests/test_save_ram.c src/storage.c -Wl,--wrap=stat -o build/test-save-ram
build/test-save-ram "$@"
clang -std=c11 -D_POSIX_C_SOURCE=200809L -O1 -g -fsanitize=address,undefined -Wall -Wextra -Werror -Isrc tests/test_storage.c src/storage.c -o build/test-storage-run
build/test-storage-run
