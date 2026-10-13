#!/usr/bin/env bash
set -euo pipefail
cd /work
mkdir -p build
clang -std=c11 -D_DEFAULT_SOURCE -DNPS64_UPDATE_DIR='"/tmp/nps64-update-test"' -O1 -g -fsanitize=address,undefined -Wall -Wextra -Werror -Isrc tests/test_updater.c src/updater.c src/update_feed.c src/sha256.c -lpthread -o build/test-updater
build/test-updater
