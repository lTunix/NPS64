#!/usr/bin/env bash
set -euo pipefail
cd /work
dir="${1:-build/hle-performance-38}"
mkdir -p "$dir"
unset NPS64_COMPARE_DIR NPS64_HLE_FRAME_HASHES NPS64_TEST_HLE_FALLBACK_FRAME NPS64_HLE_CPU_VI VK_INSTANCE_LAYERS
# The real-ROM validation builds this executable first. No paired image capture,
# validation layers or artificial frame pacing in these timing runs.
for mode in cpu vulkan-reference vulkan; do
  profile="$mode";extra=()
  if [ "$mode" = vulkan-reference ];then profile=vulkan;extra=(NPS64_HLE_CPU_VI=1);fi
  : > build/benchdata/nps64.log
  env "${extra[@]}" VK_ICD_FILENAMES=/usr/share/vulkan/icd.d/lvp_icd.x86_64.json BENCH_PROFILE_MODE="$profile" BENCH_FRAMES=4000 BENCH_SM64=1 BENCH_CPU=jit NPS64_FULL_ATLAS_UPLOAD=1 timeout 180 build/hle-game-native/frontend > "$dir/$mode-host.log" 2>&1
  cp build/benchdata/nps64.log "$dir/$mode-core.log"
done
python3 scripts/profile_hle_frames.py "$dir"
