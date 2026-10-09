#!/usr/bin/env bash
set -euo pipefail
cd /work
python3 scripts/compile_parallel_probe.py
clang-21 -std=c11 -O2 -Wall -Wextra -Werror -Isrc -Ibuild/parallel-probe \
  src/parallel_compute_probe.c tests/parallel_probe_host.c -lvulkan \
  -o build/parallel-probe/host
VK_ICD_FILENAMES=/usr/share/vulkan/icd.d/lvp_icd.x86_64.json \
VK_INSTANCE_LAYERS=VK_LAYER_KHRONOS_validation VK_LAYER_ENABLES=VK_VALIDATION_FEATURE_ENABLE_SYNCHRONIZATION_VALIDATION_EXT \
  timeout 90 build/parallel-probe/host > build/parallel-probe/host.log 2>&1
python3 - <<'PY'
from pathlib import Path
log=Path('build/parallel-probe/host.log').read_text()
assert 'VUID-' not in log and 'Validation Error' not in log, log
assert 'PASS all seven cases: 448 fenced submissions' in log
assert log.count('COMPLETE case=')==448
assert log.count('PASS case=')==7
print('PASS 448 fences / 896 dispatches, exact values and guards, Vulkan + synchronization validation; host only')
PY
tail -4 build/parallel-probe/host.log
