#!/usr/bin/env bash
set -euo pipefail
cd /work
clang -std=c11 -O1 -g -fsanitize=address,undefined -Wall -Wextra -Werror -Isrc tests/test_gsp_culldl.c -o build/test-gsp-culldl
build/test-gsp-culldl
