#!/usr/bin/env bash
set -euo pipefail
cd /work
rom="${1:?Pass local ROM path}"
dir="${2:-build/hle-comparison}"
mkdir -p "$dir"
# This runner reuses the real-core lifecycle test. Existing reports are retained.
NPS64_COMPARE_DIR="/work/$dir" BENCH_HLE_LIVE=1 BENCH_SM64=1 BENCH_FRAMES="${BENCH_FRAMES:-2600}" NPS64_FULL_ATLAS_UPLOAD=1 bash scripts/test_hle_game.sh "$rom"
python3 scripts/compare_hle_frames.py "$dir"
cp build/hle-game-native/live.log "$dir/core.log"
cp build/hle-game-native/host.log "$dir/host.log"
