#!/usr/bin/env bash
set -euo pipefail
cd /work
clang-21 -std=c11 -O2 -Wall -Wextra -Werror -DNPS64_PARALLEL_TILE_PROBE -DNPS64_GAME_BINNING_TEST \
 -Isrc -Ibuild/parallel-tile-probe -Ibuild/parallel-game \
 src/parallel_tile_probe.c tests/parallel_tile_host.c -lvulkan -o build/parallel-game/test-game-binning
VK_ICD_FILENAMES=/usr/share/vulkan/icd.d/lvp_icd.x86_64.json \
 VK_INSTANCE_LAYERS=VK_LAYER_KHRONOS_validation \
 VK_LAYER_ENABLES=VK_VALIDATION_FEATURE_ENABLE_SYNCHRONIZATION_VALIDATION_EXT \
 NPS64_TILE_CASE=4 timeout 90 build/parallel-game/test-game-binning > build/parallel-game/game-binning-test.log 2>&1
python3 - <<'PY'
from pathlib import Path
log=Path('build/parallel-game/game-binning-test.log').read_text()
assert 'VUID-' not in log and 'Validation Error' not in log
assert log.count('COMPLETE case=4')==64 and 'PASS case=4' in log
for count in (0,1,31,32,33,63,65,255,256):
 for q in (0,1):
  assert f'STRESS primitives={count} groups={(count+31)//32} quadrant_scissors={q}' in log
print('PASS production game SPIR-V: 64 stress dispatches; masks/inputs/guards; validation clean')
PY
