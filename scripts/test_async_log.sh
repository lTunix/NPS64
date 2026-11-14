#!/usr/bin/env bash
set -euo pipefail
cd /work
clang -std=c11 -D_POSIX_C_SOURCE=200809L -DNPS64_LOG_ASYNC -O1 -g -fsanitize=address,undefined -Wall -Wextra -Werror -Isrc tests/test_async_log.c src/log.c -Wl,--wrap=write -Wl,--wrap=pthread_create -lpthread -o build/test-async-log
build/test-async-log
