#!/usr/bin/env bash
set -euo pipefail
cd /work
python3 scripts/compile_hle_probe.py
clang-21 -std=c11 -O2 -Wall -Wextra -Werror -Isrc src/gpu_hle_probe.c src/hle_display_list.c tests/hle_renderer_live_test.c -lvulkan -lm -o build/hle-probe/live-test
VK_ICD_FILENAMES=/usr/share/vulkan/icd.d/lvp_icd.x86_64.json VK_INSTANCE_LAYERS=VK_LAYER_KHRONOS_validation timeout 90 build/hle-probe/live-test > build/hle-probe/live-test.log 2>&1
python3 - <<'PY'
from pathlib import Path
s=Path('build/hle-probe/live-test.log').read_text()
assert 'VUID-' not in s and 'Validation Error' not in s
assert s.count('persistent renderer ready')==2
assert s.count('atlas GPU readback bytes=1048576 mismatches=0')==2
assert 'PASS persistent renderer: 1200 frames' in s
print(s.splitlines()[-1])
PY
