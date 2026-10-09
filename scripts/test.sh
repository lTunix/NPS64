#!/usr/bin/env bash
set -euo pipefail
cd /work
mkdir -p build
bash scripts/test_frame_pacing.sh
bash scripts/test_rom_compat.sh
clang -std=c11 -O1 -g -fsanitize=address,undefined -Wall -Wextra -Werror -Isrc tests/test_cpu_game_profile.c -o build/test-cpu-game-profile
build/test-cpu-game-profile
bash scripts/test_cpu_heap.sh
clang -std=c11 -O1 -g -fsanitize=address,undefined -Wall -Wextra -Werror -Isrc tests/software_scaler_test.c -o build/software-scaler-test
build/software-scaler-test
clang -std=c11 -O1 -g -fsanitize=address,undefined -Wall -Wextra -Werror -Isrc tests/test_scanout_copy.c -o build/test-scanout-copy
build/test-scanout-copy
clang -std=c11 -g -fsanitize=address,undefined -Wall -Wextra -Werror -Isrc -idirafter toolchain/OpenOrbis/PS4Toolchain/include tests/test_input.c -o build/test-input-run
build/test-input-run
clang -std=c11 -O1 -g -fsanitize=address,undefined -Wall -Wextra -Werror -Isrc tests/test_audio_resampler.c -lm -o build/test-audio-resampler
build/test-audio-resampler
clang -std=c11 -O1 -g -fsanitize=address,undefined -Wall -Wextra -Werror -Isrc tests/test_audio_queue.c -o build/test-audio-queue
build/test-audio-queue
clang -std=c11 -D_DEFAULT_SOURCE -D_POSIX_C_SOURCE=200809L -D__ORBIS__ -DNPS64_TEST_TSC -DFONT_PATH='"assets/font.bin"' -g -fsanitize=address,undefined -Wall -Wextra -Werror -Wno-unused-parameter -Isrc -idirafter toolchain/OpenOrbis/PS4Toolchain/include tests/test_ps4_backend.c src/platform_ps4.c src/log.c -lm -lpthread -o build/test-ps4-backend
build/test-ps4-backend
TEST_TSC=1 build/test-ps4-backend
TEST_FLIP_JITTER=1 build/test-ps4-backend
TEST_AUDIO=1 build/test-ps4-backend
TEST_AUDIO=1 TEST_AUDIO_PRIORITY_DENIED=1 build/test-ps4-backend
clang -std=c11 -D_POSIX_C_SOURCE=200809L -g -fsanitize=address,undefined -Wall -Wextra -Werror -Isrc tests/test_storage.c src/storage.c -o build/test-storage-run
build/test-storage-run
mkdir -p build/native-core
tar -C vendor/parallel-n64 --exclude=.git -cf - . | tar -C build/native-core -xf -
python3 scripts/patch_core.py build/native-core
bash scripts/test_cpu_lane.sh build/native-core
make -C build/native-core -j"${JOBS:-4}" platform=unix WITH_DYNAREC= HAVE_OPENGL=0 HAVE_PARALLEL=0 HAVE_PARALLEL_RSP=0 STATIC_LINKING=1 TARGET=parallel_n64.a CPUOPTS='-O2 -DNDEBUG -DNPS64_COMPRESSED_MEMORY' GIT_VERSION=890323d CC=clang CXX=clang++ AR=llvm-ar > build/native-core.log 2>&1 || { tail -80 build/native-core.log; exit 1; }
python3 tests/make_test_rom.py
clang -std=c11 -D_POSIX_C_SOURCE=200809L -Wall -Wextra -Werror -O2 -Isrc -Ivendor/parallel-n64/libretro-common/include -g -fsanitize=address,undefined tests/test_render.c src/storage.c src/log.c tests/platform_host.c build/native-core/parallel_n64.a -lm -lpthread -o build/test-render
build/test-render
clang -std=c11 -D_POSIX_C_SOURCE=200809L -DDATA_ROOT='"/work/build/testdata"' -Wall -Wextra -Werror -O2 -Isrc -Ivendor/parallel-n64/libretro-common/include src/frontend.c src/storage.c src/log.c tests/platform_host.c build/native-core/parallel_n64.a -lm -lpthread -o build/test-frontend
for profile in 0 4; do
printf 'NPS64DIAG1 %s\n' "$profile" > build/testdata/diagnostics.cfg
: > build/testdata/nps64.log
timeout 60 build/test-frontend
python3 - <<'PY'
from pathlib import Path
p = Path('/work/build/testdata/saves')
assert list(p.glob('*.srm')), 'no SRAM file'
assert list(p.glob('*.state')), 'no state file'
log = Path('/work/build/testdata/nps64.log').read_text()
assert 'Estado guardado' in log and 'Estado cargado' in log, log[-3000:]
assert log.count('[load] hash_begin')==2, 'controller test must not launch content'
assert '[load] first_video' in log and '[run] frame=0 end' in log
assert 'Biblioteca actualizada: 1 ROMs' in log
print('PASS: SRAM y estado del nucleo real guardados/cargados')
PY
cp build/testdata/nps64.log "build/native-profile-${profile// /-}.log"
done
rm -f build/testdata/diagnostics.cfg

bash scripts/build_native_jit.sh
bash scripts/test_jit.sh
