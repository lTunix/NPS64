#!/usr/bin/env bash
set -euo pipefail
cd /work
export BENCH_CORE=build/native-jit BENCH_JIT=1 BENCH_CPU=jit BENCH_RSP=hle BENCH_RACE=1 BENCH_FRAMES=3600 BENCH_TIMEOUT=240
if [ -f build/baseline-00.05-frontend.c ] && [ -f build/baseline-00.05-core.a ]; then
BENCH_FRONTEND=build/baseline-00.05-frontend.c BENCH_ARCHIVE=build/baseline-00.05-core.a BENCH_WORKERS=4 bash scripts/bench.sh 'Mario Kart 64 (E) (V1.0) [!].z64' > build/bench-00.06-baseline05.log
cp build/benchdata/nps64.log build/bench-00.06-baseline05-core.log
cp build/bench-frame-3570.ppm build/bench-00.06-baseline05-race.ppm
tail -3 build/bench-00.06-baseline05.log
fi
for profile in "4 filtered" "4 fast" "6 fast" "2 fast"; do
read -r workers vi <<< "$profile"
BENCH_WORKERS="$workers" BENCH_VI="$vi" bash scripts/bench.sh 'Mario Kart 64 (E) (V1.0) [!].z64' > "build/bench-00.06-$vi-w$workers.log"
cp build/benchdata/nps64.log "build/bench-00.06-$vi-w$workers-core.log"
cp build/bench-frame-3570.ppm "build/bench-00.06-$vi-w$workers-race.ppm"
tail -3 "build/bench-00.06-$vi-w$workers.log"
done
