#!/usr/bin/env bash
set -euo pipefail
cd /work
rom="${1:?Pass the local ROM path}"
test -f "$rom"
mkdir -p build/benchdata/games build/benchdata/saves build/benchdata/system
ln -sf "$(realpath "$rom")" build/benchdata/games/benchmark.z64
printf 'NPS64DIAG1 %s\n' "${BENCH_WORKERS:-0}" > build/benchdata/diagnostics.cfg
extra=()
if [ "${BENCH_JIT:-0}" = 1 ]; then extra=(-DDYNAREC -DNEW_DYNAREC=2 -D_M_X64 -DARCH_MIN_SSE2); fi
if [ "${BENCH_GPROF:-0}" = 1 ]; then extra+=(-pg -g); fi
if [ "${BENCH_RDP_PROFILE:-0}" = 1 ]; then extra+=(-fprofile-instr-generate); fi
clang "${extra[@]}" -std=c11 -D_POSIX_C_SOURCE=200809L -D__LIBRETRO__ -DDATA_ROOT='"/work/build/benchdata"' -Wall -Wextra -Werror -O2 -Isrc -Ivendor/parallel-n64/libretro-common/include -Ivendor/parallel-n64/mupen64plus-core/src -Ivendor/parallel-n64/include ${BENCH_FRONTEND:-src/frontend.c} src/storage.c src/log.c tests/platform_bench.c ${BENCH_ARCHIVE:-${BENCH_CORE:-build/native-core}/parallel_n64.a} -lm -lpthread -o build/bench-frontend
: > build/benchdata/nps64.log
timeout "${BENCH_TIMEOUT:-120}" build/bench-frontend > build/bench-results.log 2>&1
python3 - <<'PY'
from pathlib import Path
log=Path('build/benchdata/nps64.log').read_text()
assert '[load] first_visible_video' in log, 'ROM never generated visible output'
PY
cat build/bench-results.log
