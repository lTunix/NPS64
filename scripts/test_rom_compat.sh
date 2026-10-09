#!/usr/bin/env bash
set -euo pipefail
cd /work
clang -std=c11 -O1 -g -fsanitize=address,undefined -Wall -Wextra -Werror -Isrc \
  tests/test_rom_compat.c -o build/test-rom-compat
build/test-rom-compat "$@"
