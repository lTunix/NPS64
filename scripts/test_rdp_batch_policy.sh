#!/usr/bin/env bash
set -euo pipefail
cd /work
clang -std=c11 -O1 -g -fsanitize=address,undefined -Wall -Wextra -Werror -Isrc tests/test_rdp_batch_policy.c -o build/test-rdp-batch-policy
build/test-rdp-batch-policy
