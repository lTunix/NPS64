#!/usr/bin/env bash
set -euo pipefail
cd /work
python3 tests/make_test_rom.py
clang-21 -std=c11 -D_GNU_SOURCE -D_POSIX_C_SOURCE=200809L -DNPS64_GPU -DNPS64_TEST_GPU_QUARANTINE -DDATA_ROOT='"/work/build/testdata"' -O2 -Wall -Wextra -Werror -Isrc -Ivendor/parallel-n64/libretro-common/include -Ivendor/parallel-n64/mupen64plus-core/src/api src/frontend.c src/gpu_vulkan.c src/storage.c src/log.c tests/platform_host.c tests/gpu_quarantine_stub.c -Wl,--wrap=nps64_gpu_begin build/native-gpu/parallel_n64.a -lstdc++ -ldl -lm -lpthread -o build/test-gpu-quarantine
printf 'NPS64DIAG5 4 1 1 1 1\n' > build/testdata/diagnostics.cfg
: > build/testdata/nps64.log
BENCH_GPU=on VK_ICD_FILENAMES=/work/build/nonexistent.json timeout 60 build/test-gpu-quarantine > build/gpu-quarantine-test.log 2>&1
cp build/testdata/nps64.log build/gpu-quarantine-core.log
python3 - <<'PYTEST'
from pathlib import Path
s=Path('build/gpu-quarantine-core.log').read_text()
assert 'PS4 Vulkan quarantined after system freeze' in s
assert s.count('renderer=Angrylion')==2
assert 'Vulkan instance ready' not in s and 'renderer=paraLLEl-RDP active' not in s
assert 'Estado guardado' in s and 'Estado cargado' in s
print('PASS: GPU requested by config and environment is blocked; no Vulkan init; two CPU sessions, image and save/load work')
PYTEST
rm -f build/testdata/diagnostics.cfg
