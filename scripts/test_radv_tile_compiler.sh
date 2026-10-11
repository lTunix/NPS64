#!/usr/bin/env bash
set -euo pipefail
cd /work
out=/work/build/parallel-tile-probe
driver=/work/build/radv-native/src/amd/vulkan/libvulkan_radeon.so
shim=/work/build/radv-native/src/amd/drm-shim/libamdgpu_noop_drm_shim.so
test -f "$driver"
test -f "$shim"
clang-21 -std=c11 -O2 -Wall -Wextra -Werror -DNPS64_PARALLEL_TILE_PROBE \
 -Isrc -I"$out" src/parallel_tile_probe.c tests/parallel_tile_host.c \
 -lvulkan -o "$out/host-aco"
printf '{"file_format_version":"1.0.0","ICD":{"library_path":"%s","api_version":"1.3.0"}}\n' \
 "$driver" > "$out/radv-native.json"
AMDGPU_GPU_ID=bonaire LD_PRELOAD="$shim" \
 LD_LIBRARY_PATH=/work/build/radv-native-deps/lib \
 VK_ICD_FILENAMES="$out/radv-native.json" NPS64_TILE_COMPILE_ONLY=1 \
 timeout 90 "$out/host-aco" > "$out/aco-bonaire.log" 2>&1
python3 - <<'PY'
from pathlib import Path
log=Path('build/parallel-tile-probe/aco-bonaire.log').read_text()
for case in (0,1,2,3,4,5,6,7):
    assert f'COMPILE ONLY case={case} complete' in log, log
assert 'SUBMIT' not in log and 'COMPLETE case=' not in log, log
assert 'PASS AMD compilation only' in log, log
print(log)
PY
