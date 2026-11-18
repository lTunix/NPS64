#!/usr/bin/env bash
set -euo pipefail
cd /work
core=${1:-build/gpu-game-test/core}
mkdir -p build/jit-delay-entry-test
python3 - "$core" <<'PY'
from pathlib import Path
import sys
s=(Path(sys.argv[1])/'mupen64plus-core/src/device/r4300/new_dynarec/new_dynarec.c').read_text()
a=s.index('/* NPS64: page-spanning delay-slot register liveness. */')
b=s.index('      current.u&=',a)
body=s[a:b]
Path('build/jit-delay-entry-test/jit_delay_entry_generated.h').write_text('struct masks { uint64_t u,uu; };\n#define NPS64_DELAY_MASKS do { \\\n'+''.join(line+' \\\n' for line in body.splitlines())+'} while(0)\n')
PY
clang-21 -std=c11 -O1 -g -fsanitize=address,undefined -Wall -Wextra -Werror \
    -Ibuild/jit-delay-entry-test tests/test_jit_delay_entry.c -o build/test-jit-delay-entry
build/test-jit-delay-entry
