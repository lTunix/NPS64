#!/usr/bin/env bash
set -euo pipefail
cd /work
clang -std=c11 -O2 -g -fsanitize=address,undefined -Wall -Wextra -Werror -Isrc tests/test_bilinear_scaler.c -lm -o build/test-bilinear-scaler
build/test-bilinear-scaler
