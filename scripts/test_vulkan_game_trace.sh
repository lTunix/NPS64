#!/usr/bin/env bash
# Requires a prepared sanitizer core and a user-supplied local ROM directory.
set -euo pipefail
cd /work
core_dir="${NATIVE_GPU_DIR:-build/native-gpu-asan}"
test_data="${NPS64_GPU_TEST_DATA:-/work/build/gpu-game-rom}"
test -f "$core_dir/parallel_n64.a"
present_flags=()
if [ "${NPS64_NATIVE_GPU_PRESENT:-0}" = 1 ]; then present_flags=(-DNPS64_GPU_PRESENT_TEST); fi
clang++-21 -resource-dir=/work/build/tools/usr/lib/llvm-18/lib/clang/18 \
  -x c "${present_flags[@]}" -DNPS64_LOG_ASYNC -mssse3 -std=c11 -D_GNU_SOURCE -D_POSIX_C_SOURCE=200809L \
  -DNPS64_GPU -DNPS64_IMAGE_SMOOTH_TEST -DNPS64_INTERNAL_2X_TEST -DNPS64_GPU_BORROW_TEST -DNPS64_GPU_GAME_TEST -DNPS64_HOST_COMPILE_WORKER_TEST -DNPS64_GAME_TEST_SECONDS="${NPS64_NATIVE_GAME_SECONDS:-60}" -DNPS64_SESSION_TEST_SECONDS="${NPS64_NATIVE_GAME_SECONDS:-60}" "-DDATA_ROOT=\"$test_data\"" \
  -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer -Wall -Wextra -Werror \
  -Isrc -Ivendor/parallel-n64/libretro-common/include \
  -Ivendor/parallel-n64/mupen64plus-core/src -Ivendor/parallel-n64/mupen64plus-core/src/api \
  -Ivendor/parallel-n64/mupen64plus-video-paraLLEl/parallel-rdp/vulkan-headers/include \
  src/gpu_presenter.c src/frontend.c src/i18n.c src/settings.c src/gpu_vulkan.c src/gpu_api_trace.c src/gpu_compile_worker.c src/gpu_cache_store.c src/gpu_recipes.c src/gpu_crashlog.c src/sha256.c src/storage.c src/log.c tests/platform_vulkan_session.c \
  -x none "$core_dir/parallel_n64.a" -fuse-ld=lld -ldl -lm -lpthread -o build/bench-vulkan-game-00.32-asan
BENCH_FRAMES=1000000 NPS64_GPU_NO_DEEPBIND=1 \
  ASAN_OPTIONS=detect_leaks=0 UBSAN_OPTIONS=print_stacktrace=1 \
  ASAN_SYMBOLIZER_PATH=/work/build/tools/usr/bin/llvm-symbolizer-18 \
  VK_DRIVER_FILES=/usr/share/vulkan/icd.d/lvp_icd.json \
  VK_LAYER_PATH=/work/build/tools/usr/share/vulkan/explicit_layer.d \
  VK_INSTANCE_LAYERS=VK_LAYER_KHRONOS_validation \
  timeout 180 build/bench-vulkan-game-00.32-asan > build/vulkan-game-00.32-asan.log 2>&1
python3 - "$test_data" "${NPS64_NATIVE_GAME_SECONDS:-60}" "${NPS64_NATIVE_SCALE:-2}" <<'PY'
from pathlib import Path
import json, sys, re
sys.path.insert(0, 'scripts')
from analyze_gpu_checkpoints import analyze
data = Path(sys.argv[1])
text = (data / 'vulkan-game-test.log').read_text().split('NPS64 Vulkan Game Test 00.32;')[-1]
driver = Path('build/vulkan-game-00.32-asan.log').read_text()
report = analyze('NPS64 Vulkan Game Test 00.32;' + text + '\n' + driver)
assert not any('runtime error:' in line and ('rdp_renderer.cpp' in line or 'bilinear_scaler.h' in line) for line in driver.splitlines())
assert report['entries'] == 0 and not report['pending_calls'] and not report['log_errors']
limits = re.findall(r'\[gpu-test\] LIMIT reached elapsed_s=([\d.]+) VI=(\d+);', text)
stops = re.findall(r'\[gpu-test\] stop VI=(\d+) elapsed_s=([\d.]+) limit_reached=(\d+)', text)
assert len(limits) == 1 and len(stops) == 1, (limits, stops)
assert float(limits[0][0]) >= float(sys.argv[2]) and int(limits[0][1]) > 180
assert limits[0][1] == stops[0][0] and stops[0][2] == '1'
assert '[session-test] attempt resume after deadline' in text
assert '[rdp-checkpoint]' not in driver and '[gpu-api]' not in text and '[gpu-api-error]' not in text
assert driver.count('[pipeline-compile] begin') > 0
assert driver.count('[pipeline-compile] begin') == driver.count('[pipeline-compile] end')
if sys.argv[3]=='2':
    assert 'Using 2x upscaling!' in text
    assert any(int(w)>640 for w in re.findall(r'\[gpu\] scanout geometry (\d+)x',text)), 'No upscaled scanout'
else:
    assert 'Using 2x upscaling!' not in text
    assert max(map(int,re.findall(r'\[gpu\] scanout geometry (\d+)x',text)))<=640
presentation=__import__('os').environ.get('NPS64_NATIVE_GPU_PRESENT','0')=='1'
assert ('storage=gpu-composited' if presentation else 'storage=borrowed-vulkan') in text
assert '[log-queue] async=1 dropped=0' in text
assert not re.search(r'\[log-queue\].*dropped=[1-9]',text), 'Logging queue overflow'
assert '[cadence]' in text and 'longest_ms=' in text and 'includes_sleep=1' in text
assert '[rdp-times]' in text and 'list_includes_fullsync=1' in text
rdp_intervals=[dict(re.findall(r'(\w+)=([^\s]+)',line)) for line in text.splitlines() if line.startswith('[rdp-times]')]
assert any(int(row['fullsync_calls'])>0 for row in rdp_intervals)
assert '[rdp-policy] fullsync=disabled' not in text
for row in rdp_intervals:
    assert float(row['list_ms'])>=0 and float(row['vi_ms'])>=0 and float(row['fullsync_ms'])>=0
    assert float(row['fullsync_ms'])<=float(row['list_ms'])+.002
assert '[frame-times]' in text and '[perf]' in text
if presentation:
    assert '[gpu-present] enabled=1' in text
    assert '[vulkan-times]' not in text, 'Unexpected legacy readback'
    rows=[dict(re.findall(r'(\w+)=([^\s]+)',line)) for line in text.splitlines() if line.startswith('[gpu-present-times]')]
    assert len(rows)>10
    assert all(int(row['compositions'])>0 for row in rows)
    assert all(float(row[key])>=0 for row in rows for key in ['acquire_ms','record_ms','submit_ms','fence_wait_ms','cpu_copy_ms'])
    report['composition_intervals']=len(rows)
else:
    assert '[vulkan-times]' in text and '[gpu-readback] memory_type=' in text
    assert 'cached=1' in text
    transfers = [dict(re.findall(r'(\w+)=([^\s]+)', line)) for line in text.splitlines() if line.startswith('[vulkan-times]')]
    assert len(transfers) > 10
    for transfer in transfers:
        values = {key: float(transfer[key]) for key in ['record_ms','submit_ms','fence_wait_ms','invalidate_ms','readback_ms','snapshot_ms']}
        assert values['snapshot_ms']==0, 'Redundant Vulkan snapshot copy'
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
(out / 'host-00.32-checkpoints.json').write_text(json.dumps(report, indent=2))
(out / 'host-00.32-core.log').write_text('NPS64 Vulkan Game Test 00.32;' + text)
print('PASS: split RDP beyond 180 VI, timed cap, resume prevented, compact metrics; no ASan or Vulkan validation error')
print(f'UBSan reports outside Vulkan retained for triage: {len(reports)}; this is not a clean UBSan run')
PY
