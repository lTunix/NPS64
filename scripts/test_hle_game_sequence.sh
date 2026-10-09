#!/usr/bin/env bash
set -euo pipefail
cd /work
python3 scripts/compile_hle_probe.py
clang-21 -std=c11 -O2 -Wall -Wextra -Werror -Isrc src/gpu_hle_probe.c src/hle_display_list.c tests/hle_game_sequence_test.c -lvulkan -lm -o build/hle-probe/sequence-test
VK_ICD_FILENAMES=/usr/share/vulkan/icd.d/lvp_icd.x86_64.json VK_INSTANCE_LAYERS=VK_LAYER_KHRONOS_validation timeout 60 build/hle-probe/sequence-test > build/hle-probe/sequence-test.log 2>&1
tail -4 build/hle-probe/sequence-test.log
python3 - <<'PY'
from pathlib import Path
s=Path('build/hle-probe/sequence-test.log').read_text()
assert 'VUID-' not in s and 'Validation Error' not in s
assert 'frames=120 changed=119' in s
assert 'frames=7 changed=6' in s
assert 'scene_frames=6 uniform_frames=2' in s
assert 'FAIL Vulkan sequence frames=8 changed=0' in s
assert 'PASS 120 changing frames' in s
PY
