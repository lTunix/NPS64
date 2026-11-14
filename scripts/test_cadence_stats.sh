#!/usr/bin/env bash
set -euo pipefail
cd /work
clang -std=c11 -O1 -g -fsanitize=address,undefined -Wall -Wextra -Werror -Isrc tests/test_cadence_stats.c -lm -o build/test-cadence-stats
build/test-cadence-stats
