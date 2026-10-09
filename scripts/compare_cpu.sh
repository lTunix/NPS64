#!/usr/bin/env bash
set -euo pipefail
cd /work
export BENCH_CORE=build/native-jit BENCH_JIT=1 BENCH_RSP=hle BENCH_RACE=1 BENCH_FRAMES=3600 BENCH_TIMEOUT=240
for cpu in cached jit; do
BENCH_CPU="$cpu" bash scripts/bench.sh 'Mario Kart 64 (E) (V1.0) [!].z64' > "build/bench-00.06-$cpu.log"
cp build/benchdata/nps64.log "build/bench-00.06-$cpu-core.log"
cp build/bench-frame-3570.ppm "build/bench-00.06-$cpu-race.ppm"
tail -3 "build/bench-00.06-$cpu.log"
done
BENCH_CPU=jit BENCH_WORKERS=4 bash scripts/bench.sh 'Mario Kart 64 (E) (V1.0) [!].z64' > build/bench-00.06-jit-workers4.log
cp build/benchdata/nps64.log build/bench-00.06-jit-workers4-core.log
tail -3 build/bench-00.06-jit-workers4.log
