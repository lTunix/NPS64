#!/usr/bin/env bash
set -euo pipefail
cd /work
core_dir="${NATIVE_GPU_DIR:-build/native-gpu-asan}"
test_data=/work/build/live-resolution-37/integration
: "${NPS64_RESOLUTION_ROM:?Set a private N64 ROM path}"
mkdir -p "$test_data/games"
ln -sf "$NPS64_RESOLUTION_ROM" "$test_data/games/Game.z64"
rm -f "$test_data/vulkan-game-test.log"
present_flags=(-DNPS64_GPU_PRESENT_TEST)
clang++-21 -Wl,--wrap=retro_run -Wl,--wrap=nps64_parallel_resolution -resource-dir=/work/build/tools/usr/lib/llvm-18/lib/clang/18 \
  -x c "${present_flags[@]}" -DNPS64_LOG_ASYNC -mssse3 -std=c11 -D_GNU_SOURCE -D_POSIX_C_SOURCE=200809L \
  -DNPS64_CHEAT_DATABASE="\"/work/build/cheats-33/cheats.db\"" -DNPS64_GPU -DNPS64_IMAGE_SMOOTH_TEST -DNPS64_INTERNAL_2X_TEST -DNPS64_GPU_BORROW_TEST -DNPS64_GPU_GAME_TEST -DNPS64_HOST_COMPILE_WORKER_TEST -DNPS64_GAME_TEST_SECONDS="120" -DNPS64_SESSION_TEST_SECONDS="120" "-DDATA_ROOT=\"$test_data\"" \
  -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer -Wall -Wextra -Werror \
  -Isrc -Ivendor/parallel-n64/libretro-common/include \
  -Ivendor/parallel-n64/mupen64plus-core/src -Ivendor/parallel-n64/mupen64plus-core/src/api \
  -Ivendor/parallel-n64/mupen64plus-video-paraLLEl/parallel-rdp/vulkan-headers/include \
  src/gpu_presenter.c src/frontend.c src/i18n.c src/cheats.c src/settings.c src/gpu_vulkan.c src/gpu_api_trace.c src/gpu_compile_worker.c src/gpu_cache_store.c src/gpu_recipes.c src/gpu_crashlog.c src/sha256.c src/storage.c src/log.c tests/platform_resolution_session.c \
  -x none "$core_dir/parallel_n64.a" -fuse-ld=lld -ldl -lm -lpthread -o build/live-resolution-37/test-native

BENCH_FRAMES=1000000 NPS64_GPU_NO_DEEPBIND=1 ASAN_OPTIONS=detect_leaks=0 UBSAN_OPTIONS=print_stacktrace=1 \
 VK_DRIVER_FILES=/usr/share/vulkan/icd.d/lvp_icd.json VK_LAYER_PATH=/work/build/tools/usr/share/vulkan/explicit_layer.d \
 VK_INSTANCE_LAYERS=VK_LAYER_KHRONOS_validation timeout 180 build/live-resolution-37/test-native > build/live-resolution-37/integration.log 2>&1
python3 - <<'CHECK'
from pathlib import Path
s=Path('build/live-resolution-37/integration/vulkan-game-test.log').read_text()
assert s.count('[resolution-test] PASS factor=')==6 and 'six live switches' in s
assert s.count('[load] retro_load_game_ok')==1
log=Path('build/live-resolution-37/integration.log').read_text()
assert 'ERROR: AddressSanitizer' not in log and 'Validation Error' not in log and 'VUID-' not in log
print('PASS six live resolution switches, one ROM load, RAM/SRAM unchanged and no ASan/Vulkan validation errors')
CHECK
