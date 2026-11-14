#!/usr/bin/env bash
set -euo pipefail
cd /work
python3 tests/make_test_rom.py
clang -mssse3 -std=c11 -D_POSIX_C_SOURCE=200809L -DNPS64_INTERNAL_2X_TEST -DNPS64_HD_UI -DNPS64_UI_VERSION='"00.29"' -DNPS64_ASSET_ROOT='"/work/build/stage"' -DDATA_ROOT='"/work/build/ui-settings-test"' -O1 -g -fsanitize=address,undefined -Wall -Wextra -Werror -Isrc -Ivendor/parallel-n64/libretro-common/include tests/test_frontend_settings.c src/settings.c src/storage.c src/library_ui.c src/cover_match.c src/log.c build/native-core/parallel_n64.a -fuse-ld=lld -lm -lpthread -o build/test-frontend-settings
ASAN_OPTIONS=detect_leaks=0 timeout 40 build/test-frontend-settings
clang -mssse3 -std=c11 -g -fsanitize=address,undefined -Wall -Wextra -Werror -Isrc -idirafter toolchain/OpenOrbis/PS4Toolchain/include tests/test_input.c -o build/test-input-run
build/test-input-run
