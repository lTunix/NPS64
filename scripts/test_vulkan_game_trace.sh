#!/usr/bin/env bash
# Requires a prepared sanitizer core and a user-supplied local ROM directory.
set -euo pipefail
cd /work
core_dir="${NATIVE_GPU_DIR:-build/native-gpu-asan}"
test_data="${NPS64_GPU_TEST_DATA:-/work/build/gpu-game-rom}"
test -f "$core_dir/parallel_n64.a"
clang++-21 -resource-dir=/work/build/tools/usr/lib/llvm-18/lib/clang/18 \
  -x c -std=c11 -D_GNU_SOURCE -D_POSIX_C_SOURCE=200809L \
  -DNPS64_GPU -DNPS64_GPU_GAME_TEST -DNPS64_HOST_COMPILE_WORKER_TEST -DNPS64_GAME_TEST_SECONDS="${NPS64_NATIVE_GAME_SECONDS:-30}" -DNPS64_SESSION_TEST_SECONDS=30 "-DDATA_ROOT=\"$test_data\"" \
  -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer -Wall -Wextra -Werror \
  -Isrc -Ivendor/parallel-n64/libretro-common/include \
  -Ivendor/parallel-n64/mupen64plus-core/src -Ivendor/parallel-n64/mupen64plus-core/src/api \
  -Ivendor/parallel-n64/mupen64plus-video-paraLLEl/parallel-rdp/vulkan-headers/include \
  src/frontend.c src/settings.c src/gpu_vulkan.c src/gpu_api_trace.c src/gpu_compile_worker.c src/gpu_cache_store.c src/gpu_recipes.c src/gpu_crashlog.c src/sha256.c src/storage.c src/log.c tests/platform_vulkan_session.c \
  -x none "$core_dir/parallel_n64.a" -fuse-ld=lld -ldl -lm -lpthread -o build/bench-vulkan-game-00.14-asan
BENCH_FRAMES=1000000 NPS64_GPU_NO_DEEPBIND=1 \
  ASAN_OPTIONS=detect_leaks=0 UBSAN_OPTIONS=print_stacktrace=1 \
  ASAN_SYMBOLIZER_PATH=/work/build/tools/usr/bin/llvm-symbolizer-18 \
  VK_DRIVER_FILES=/usr/share/vulkan/icd.d/lvp_icd.json \
  VK_LAYER_PATH=/work/build/tools/usr/share/vulkan/explicit_layer.d \
  VK_INSTANCE_LAYERS=VK_LAYER_KHRONOS_validation \
  timeout 180 build/bench-vulkan-game-00.14-asan > build/vulkan-game-00.14-asan.log 2>&1
python3 - "$test_data" <<'PY'
from pathlib import Path
import json, sys, re
sys.path.insert(0, 'scripts')
from analyze_gpu_checkpoints import analyze
data = Path(sys.argv[1])
text = (data / 'vulkan-game-test.log').read_text().split('NPS64 Vulkan Game Test 00.14;')[-1]
driver = Path('build/vulkan-game-00.14-asan.log').read_text()
report = analyze('NPS64 Vulkan Game Test 00.14;' + text + '\n' + driver)
assert report['entries'] == 0 and not report['pending_calls'] and not report['log_errors']
limits = re.findall(r'\[gpu-test\] LIMIT reached elapsed_s=([\d.]+) VI=(\d+);', text)
stops = re.findall(r'\[gpu-test\] stop VI=(\d+) elapsed_s=([\d.]+) limit_reached=(\d+)', text)
assert len(limits) == 1 and len(stops) == 1, (limits, stops)
assert float(limits[0][0]) >= 30 and int(limits[0][1]) > 180
assert limits[0][1] == stops[0][0] and stops[0][2] == '1'
assert '[session-test] attempt resume after deadline' in text
assert '[rdp-checkpoint]' not in driver and '[gpu-api]' not in text and '[gpu-api-error]' not in text
assert driver.count('[pipeline-compile] begin') > 0
assert driver.count('[pipeline-compile] begin') == driver.count('[pipeline-compile] end')
assert '[frame-times]' in text and '[perf]' in text
assert '[vulkan-times]' in text and '[gpu-readback] memory_type=' in text
assert 'cached=1' in text
transfers = [dict(re.findall(r'(\w+)=([^\s]+)', line)) for line in text.splitlines() if line.startswith('[vulkan-times]')]
assert len(transfers) > 10
for transfer in transfers:
    values = {key: float(transfer[key]) for key in ['record_ms','submit_ms','fence_wait_ms','invalidate_ms','readback_ms','snapshot_ms']}
    assert all(value >= 0 for value in values.values())
    assert abs(values['readback_ms'] - sum(values[key] for key in ['record_ms','submit_ms','fence_wait_ms','invalidate_ms'])) < 0.005
report['transfer_intervals'] = len(transfers)

assert text.count('[compiler-worker] joined') > 0
assert '[pipeline-cache] init' in driver
assert '[pipeline-cache] init loaded=0 bytes=0 ready=1' in driver
assert '[pipeline-cache] save skipped:' in driver
assert '[pipeline-cache-policy] raw_binary_restore=0 portable_recipes=0 hybrid_async=1' in text
assert '[pipeline-recipes]' not in driver
assert 'ACO_DEBUG=nosched' in text
assert '[pipeline-hybrid] generic ready=1' in driver
assert '[pipeline-async] enqueue' in driver
report['raw_binary_restore']=False
assert 'sync_shader=' + __import__('os').environ.get('NPS64_RDP_SYNC_SHADER', '0') in text
assert '[compiler-worker] launch' not in text and '[compiler-worker] driver begin' not in text
report['session_limit'] = {'elapsed_s': float(limits[0][0]), 'vi': int(limits[0][1]), 'resume_prevented': True}
assert '[gpu] first scanout' in text and 'ubershader=0' in text
assert 'ERROR: AddressSanitizer' not in driver
assert 'Validation Error' not in driver and 'VUID-' not in driver
reports = [line for line in driver.splitlines() if 'runtime error:' in line]
assert not any('parallel-rdp/vulkan/' in line for line in reports), 'Vulkan UBSan report remains'
report['remaining_ubsan_reports'] = reports
out = Path('build/gpu-game-test')
out.mkdir(exist_ok=True)
(out / 'host-00.14-checkpoints.json').write_text(json.dumps(report, indent=2))
(out / 'host-00.14-core.log').write_text('NPS64 Vulkan Game Test 00.14;' + text)
print('PASS: split RDP beyond 180 VI, timed cap, resume prevented, compact metrics; no ASan or Vulkan validation error')
print(f'UBSan reports outside Vulkan retained for triage: {len(reports)}; this is not a clean UBSan run')
PY
