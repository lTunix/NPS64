#!/usr/bin/env bash
set -euo pipefail
cd /work
python3 scripts/compile_hle_probe.py
clang-14 -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined -Isrc -Ivendor/parallel-n64/mupen64plus-video-angrylion src/hle_game_geometry.c src/hle_game_material.c tests/hle_game_material_test.c -lm -o build/hle-probe/material-test
build/hle-probe/material-test
clang-21 -std=c11 -O2 -Wall -Wextra -Werror -Isrc src/gpu_hle_probe.c src/hle_display_list.c tests/hle_game_texture_test.c -lvulkan -lm -o build/hle-probe/texture-test
VK_ICD_FILENAMES=/usr/share/vulkan/icd.d/lvp_icd.x86_64.json VK_INSTANCE_LAYERS=VK_LAYER_KHRONOS_validation timeout 60 build/hle-probe/texture-test > build/hle-probe/texture-test.log 2>&1
tail -3 build/hle-probe/texture-test.log
python3 - <<'PY'
from pathlib import Path
s=Path('build/hle-probe/texture-test.log').read_text()
assert 'VUID-' not in s and 'Validation Error' not in s
assert 'PASS atlas origin' in s
PY
