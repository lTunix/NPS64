#!/usr/bin/env bash
set -euo pipefail
cd /work
python3 scripts/compile_hle_probe.py
clang-14 -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined -Isrc src/hle_display_list.c tests/hle_display_list_test.c -lm -o build/hle-probe/display-list-test
build/hle-probe/display-list-test
clang-21 -std=c11 -O2 -Wall -Wextra -Werror -Isrc src/gpu_hle_probe.c src/hle_display_list.c tests/hle_probe_host.c -lvulkan -o build/hle-probe/host
VK_ICD_FILENAMES=/usr/share/vulkan/icd.d/lvp_icd.x86_64.json VK_INSTANCE_LAYERS=VK_LAYER_KHRONOS_validation timeout 30 build/hle-probe/host > build/hle-probe/host.log 2>&1
tail -4 build/hle-probe/host.log
python3 - <<'PY'
from pathlib import Path
s=Path('build/hle-probe/host.log').read_text()
assert 'VUID-' not in s and 'Validation Error' not in s
assert 'PASS full framebuffer' in s
PY
echo 'PASS graphics probe with Vulkan validation; host only'
