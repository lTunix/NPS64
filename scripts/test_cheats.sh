#!/usr/bin/env bash
set -euo pipefail
cd /work
mkdir -p build/cheats-33
python3 scripts/compile_cheat_database.py assets/mupencheat.txt build/cheats-33/cheats.db
clang -std=c11 -D_POSIX_C_SOURCE=200809L -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer -Wall -Wextra -Werror -Isrc tests/test_cheats.c src/cheats.c src/storage.c -o build/cheats-33/test-cheats
ASAN_OPTIONS=detect_leaks=0 build/cheats-33/test-cheats
python3 tests/test_cheat_parser.py
clang -std=c11 -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer -Wall -Wextra -Werror build/cheats-33/test-core-parser.c -o build/cheats-33/test-core-parser
build/cheats-33/test-core-parser
python3 tests/test_cheat_memory.py
clang -std=c11 -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer -Wall -Wextra -Werror build/cheats-33/test-core-memory.c -o build/cheats-33/test-core-memory
build/cheats-33/test-core-memory
python3 tests/test_i18n_formats.py
