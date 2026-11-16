#!/usr/bin/env bash
set -euo pipefail
cd /work
clang -std=c11 -O1 -g -fsanitize=address,undefined -Wall -Wextra -Werror -Isrc tests/test_rdp_update_window.c -o build/test-rdp-update-window
build/test-rdp-update-window
