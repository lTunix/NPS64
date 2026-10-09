#!/usr/bin/env bash
set -euo pipefail
cd /work
python3 tests/make_test_rom.py
clang-21 -std=c11 -D_GNU_SOURCE -D_POSIX_C_SOURCE=200809L -DNPS64_GPU -DNPS64_TEST_PS4_GPU_PROFILE -DDATA_ROOT='"/work/build/testdata"' -O2 -Wall -Wextra -Werror -Isrc -Ivendor/parallel-n64/libretro-common/include -Ivendor/parallel-n64/mupen64plus-core/src/api src/frontend.c src/gpu_vulkan.c src/storage.c src/log.c tests/platform_host.c tests/gpu_ready_wrap.c -Wl,--wrap=nps64_parallel_ready build/native-gpu/parallel_n64.a -lstdc++ -ldl -lm -lpthread -o build/test-gpu
printf 'NPS64DIAG5 4 1 1 1 1\n' > build/testdata/diagnostics.cfg
: > build/testdata/nps64.log
VK_ICD_FILENAMES=/usr/share/vulkan/icd.d/lvp_icd.x86_64.json timeout 120 build/test-gpu > build/gpu-functional-test.log 2>&1
cp build/testdata/nps64.log build/gpu-functional-core.log
python3 - <<'PY'
from pathlib import Path
import os
s=Path('build/gpu-functional-core.log').read_text()
assert s.count('renderer=paraLLEl-RDP active')==2
assert s.count('[gpu] first scanout')==2
assert s.count('PS4 compute profile: ubershader=1 direct dispatch; subgroup=0 small_types=0')==2
renderer_log=Path('build/gpu-functional-test.log').read_text()
assert renderer_log.count('Overriding ubershader = 1')==2
assert renderer_log.count('Allow subgroups = 0')==2
if os.environ.get('NPS64_RDP_SERIALIZE_SETUP') == '1':
 assert renderer_log.count('NPS64 diagnostic: serialize span/tile setup = 1') == 2
assert 'Estado guardado' in s and 'Estado cargado' in s
assert 'startup failed' not in s
print('PASS: Vulkan scanout, RGBA conversion, save/load state and two independent GPU sessions')
PY
# Force an unavailable loader driver: startup must return to Angrylion safely.
: > build/testdata/nps64.log
VK_ICD_FILENAMES=/work/build/gpu/nonexistent.json timeout 60 build/test-gpu > build/gpu-fallback-test.log 2>&1
cp build/testdata/nps64.log build/gpu-fallback-core.log
python3 - <<'PY'
from pathlib import Path
s=Path('build/gpu-fallback-core.log').read_text()
assert s.count('renderer=Angrylion')==2
assert 'renderer=paraLLEl-RDP active' not in s
print('PASS: unavailable Vulkan driver falls back to working CPU renderer')
PY
: > build/testdata/nps64.log
NPS64_TEST_GPU_NOT_READY=1 VK_ICD_FILENAMES=/usr/share/vulkan/icd.d/lvp_icd.x86_64.json timeout 120 build/test-gpu > build/gpu-device-fallback-test.log 2>&1
cp build/testdata/nps64.log build/gpu-device-fallback-core.log
python3 - <<'PY'
from pathlib import Path
s=Path('build/gpu-device-fallback-core.log').read_text()
assert s.count('startup failed; reloading with Angrylion')==2
assert s.count('renderer=Angrylion')==2
assert 'Estado guardado' in s and 'Estado cargado' in s
print('PASS: initialized Vulkan device/context teardown and CPU reload when compute renderer is unusable')
PY
python3 - <<'PY'
from pathlib import Path
for name in ['gpu-functional-test.log','gpu-device-fallback-test.log']:
 s=Path('build',name).read_text()
 assert 'Validation Error' not in s and 'VUID-' not in s, name
print('PASS: no Vulkan validation errors in functional and device-fallback tests')
PY
rm -f build/testdata/diagnostics.cfg
