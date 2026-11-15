#!/usr/bin/env bash
set -euo pipefail
cd /work
python3 tests/test_i18n_formats.py
clang -std=c11 -D_POSIX_C_SOURCE=200809L -O1 -g -fsanitize=address,undefined -Wall -Wextra -Werror -Isrc tests/test_settings.c src/settings.c src/storage.c -o build/test-settings
build/test-settings
