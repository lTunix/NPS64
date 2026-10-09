#!/usr/bin/env bash
set -euo pipefail
cd /work
clang -std=c11 -g -fsanitize=address,undefined -Wall -Wextra -Werror -Isrc tests/test_jit_memory.c src/runtime_ps4.c -lpthread -o build/test-jit-memory
build/test-jit-memory
python3 tests/make_test_rom.py
clang -std=c11 -D_POSIX_C_SOURCE=200809L -DDATA_ROOT='"/work/build/testdata"' -Wall -Wextra -Werror -O2 -Isrc -Ivendor/parallel-n64/libretro-common/include src/frontend.c src/storage.c src/log.c tests/platform_host.c ${JIT_ARCHIVE:-build/native-jit/parallel_n64.a} -lm -lpthread -o build/test-frontend-jit
for profile in "0 0" "1 0" "1 4"; do
read -r cpu workers <<< "$profile"
printf 'NPS64DIAG3 %s 1 %s\n' "$workers" "$cpu" > build/testdata/diagnostics.cfg
: > build/testdata/nps64.log
timeout 60 build/test-frontend-jit
CPU="$cpu" python3 - <<'PY'
from pathlib import Path
import os
s=Path('build/testdata/nps64.log').read_text()
assert s.count('[load] hash_begin')==2
assert 'Estado guardado' in s and 'Estado cargado' in s
assert '[load] first_video' in s
expected='selected=Ari64' if os.environ['CPU']=='1' else 'selected=cached interpreter'
assert s.count(expected)==2,s[-4000:]
print('PASS: CPU profile '+os.environ['CPU']+' state save/load and two content sessions')
PY
cp build/testdata/nps64.log "build/jit-profile-$cpu-workers-$workers.log"
done
rm -f build/testdata/diagnostics.cfg

TEST_PERF_HUD=1 build/test-frontend-jit
