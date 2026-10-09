#!/usr/bin/env bash
set -euo pipefail
cd /work
python3 scripts/compile_hle_probe.py
clang-21 -std=c11 -O2 -Wall -Wextra -Werror -Isrc src/gpu_hle_probe.c src/hle_display_list.c tests/hle_game_depth_test.c -lvulkan -o build/hle-probe/depth-test
VK_ICD_FILENAMES=/usr/share/vulkan/icd.d/lvp_icd.x86_64.json VK_INSTANCE_LAYERS=VK_LAYER_KHRONOS_validation timeout 60 build/hle-probe/depth-test > build/hle-probe/depth-test.log 2>&1
tail -4 build/hle-probe/depth-test.log
python3 - <<'PY'
from pathlib import Path
s=Path('build/hle-probe/depth-test.log').read_text()
assert 'VUID-' not in s and 'Validation Error' not in s
assert 'PASS depth-disabled background, ordered overlay and uniform snapshot rejection' in s
PY
