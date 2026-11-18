#!/usr/bin/env bash
set -euo pipefail
cd /work
core=${1:-build/gpu-game-test/core}
mkdir -p build/jit-mi-window-test
python3 - "$core" <<'PY'
from pathlib import Path
import sys
text=(Path(sys.argv[1])/'mupen64plus-core/src/device/r4300/new_dynarec/new_dynarec.c').read_text()
start=text.index('static void write_word_new(int pcaddr, int count)\n{')
end=text.index('\nstatic void write_dword_new(',start)
Path('build/jit-mi-window-test/jit_mi_store_generated.h').write_text(text[start:end])
PY
clang -std=c11 -O1 -g -fsanitize=address,undefined -Wall -Wextra -Werror -Ibuild/jit-mi-window-test tests/test_jit_mi_window.c -o build/jit-mi-window-test/test
build/jit-mi-window-test/test
