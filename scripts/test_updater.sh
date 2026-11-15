#!/usr/bin/env bash
set -euo pipefail
cd /work
mkdir -p build
# Fixtures model an upgrade from 00.68 to 00.69, independent of the app version.
clang -std=c11 -D_DEFAULT_SOURCE -DNPS64_CURRENT_TAG='"v0.68.0"' -DNPS64_UPDATE_DIR='"/tmp/nps64-update-test"' -O1 -g -fsanitize=address,undefined -Wall -Wextra -Werror -Isrc tests/test_updater.c src/i18n.c src/updater.c src/update_feed.c src/sha256.c -lpthread -o build/test-updater
build/test-updater
