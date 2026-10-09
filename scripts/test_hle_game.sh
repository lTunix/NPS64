#!/usr/bin/env bash
set -euo pipefail
cd /work
rom="${1:?Pass the local ROM path}"
dir=build/hle-game-native
mkdir -p "$dir/core/mupen64plus-video-angrylion" build/benchdata/games build/benchdata/saves build/benchdata/system
cp vendor/parallel-n64/mupen64plus-video-angrylion/rdp_emit_bridge.c "$dir/core/mupen64plus-video-angrylion/"
python3 scripts/patch_hle_geometry.py "$dir/core"
clang -std=c99 -O2 -Ivendor/parallel-n64/mupen64plus-video-angrylion -c "$dir/core/mupen64plus-video-angrylion/rdp_emit_bridge.c" -o "$dir/rdp_emit_bridge.o"
clang -std=c99 -O2 -fno-strict-aliasing -Ivendor/parallel-n64/mupen64plus-core/src -Ivendor/parallel-n64/libretro-common/include -Ivendor/parallel-n64/mupen64plus-video-angrylion -Ivendor/parallel-n64/mupen64plus-video-angrylion/n64video -c "$dir/core/mupen64plus-video-angrylion/n64video.c" -o "$dir/n64video.o"
cp build/native-jit/parallel_n64.a "$dir/core.a"
llvm-ar r "$dir/core.a" "$dir/rdp_emit_bridge.o" "$dir/n64video.o"
python3 scripts/compile_hle_probe.py
clang -DDYNAREC -DNEW_DYNAREC=2 -D_M_X64 -DARCH_MIN_SSE2 -DNPS64_HLE_GAME -std=c11 -D_POSIX_C_SOURCE=200809L -D__LIBRETRO__ -DDATA_ROOT='"/work/build/benchdata"' -Wall -Wextra -Werror -O2 -Isrc -Ivendor/parallel-n64/libretro-common/include -Ivendor/parallel-n64/mupen64plus-core/src -Ivendor/parallel-n64/include -Ivendor/parallel-n64/mupen64plus-video-angrylion src/frontend.c src/storage.c src/log.c src/hle_game_geometry.c src/hle_game_material.c src/hle_display_list.c src/gpu_hle_probe.c tests/platform_bench.c "$dir/core.a" -lvulkan -lm -lpthread -o "$dir/frontend"
ln -sf "$(realpath "$rom")" build/benchdata/games/benchmark.z64
: > build/benchdata/nps64.log
VK_ICD_FILENAMES=/usr/share/vulkan/icd.d/lvp_icd.x86_64.json VK_INSTANCE_LAYERS=VK_LAYER_KHRONOS_validation BENCH_HLE_PREVIEW=1 BENCH_FRAMES=${BENCH_FRAMES:-330} BENCH_CPU=jit timeout 180 "$dir/frontend" > "$dir/host.log" 2>&1
if [ "${BENCH_EXPECT_NO_GEOMETRY:-0}" = 1 ];then
python3 - <<'PY'
from pathlib import Path
s=Path('build/benchdata/nps64.log').read_text()
assert s.count('capture failed frame=0 after 6 VI: vertices=0')==2
assert s.count('snapshot returned=2')==2
assert 'La prueba Vulkan se permite una vez' not in s
assert 'R4300 emulator finished' in s
h=Path('build/hle-game-native/host.log').read_text()
assert '[hle-probe] graphics device=' not in h
print('PASS empty 2D/VI capture: two retries, precise log, no Vulkan resources and clean game exit')
PY
exit 0
fi
if [ "${BENCH_HLE_LIVE:-0}" = 1 ];then
python3 - <<'PY'
from pathlib import Path
import re
s=Path('build/benchdata/nps64.log').read_text()
h=Path('build/hle-game-native/host.log').read_text()
assert s.count('[hle-live] mode=Vulkan')==3
assert s.count('[hle-live] mode=CPU')==1
assert h.count('persistent renderer ready')==1
assert h.count('[hle-probe] graphics device=')==1
assert '[hle-live] renderer failed' not in s
counts=[int(n) for n in re.findall(r'\[hle-live\] GPU frames=(\d+)',s)]
assert counts and max(counts)>=400,counts
assert 'unchanged RDP repeated_frames=' in s
assert 'Estado guardado' in s and 'Estado cargado' in s
assert int(re.search(r'BENCH live_audio_frames=(\d+)',h)[1])>100000
assert s.count('R4300 emulator finished')==2
assert 'VUID-' not in h and 'Validation Error' not in h
print('PASS real ROM continuous Vulkan: 400+ new frames, duplicate VI, one context across two games, audio, pause, states and CPU/Vulkan toggles; host only')
PY
cp build/benchdata/nps64.log "$dir/live.log"
exit 0
fi
python3 - <<'PY'
from pathlib import Path
s=Path('build/benchdata/nps64.log').read_text()
assert '[load] first_visible_video' in s
assert '[hle-game] snapshot returned=0' in s
import re
matches=re.findall(r'TMEM matched=(\d+) unique_textures=(\d+) replayed_triangles=(\d+)',s)
assert any(int(a)>0 and int(b)>0 and int(c)>0 for a,b,c in matches),matches
validation=Path('build/hle-game-native/host.log').read_text()
sequence=re.search(r'PASS Vulkan sequence frames=64 changed=(\d+)',validation)
assert sequence and int(sequence[1])>0,validation[-2000:]
assert validation.count('[hle-probe] graphics device=')==1
assert re.search(r'atlas GPU readback bytes=1048576 mismatches=0 colored_texels=[1-9]\d*',validation)
assert 'VUID-' not in validation and 'Validation Error' not in validation
print('PASS local ROM: 64 changing Vulkan frames, one device, texture updates and return to game; host only')
PY
cp build/benchdata/nps64.log "$dir/cached.log"
cp build/bench-frame-999.ppm "$dir/cached-first.ppm"
cp build/bench-frame-1000.ppm "$dir/cached-last.ppm"
: > build/benchdata/nps64.log
VK_ICD_FILENAMES=/usr/share/vulkan/icd.d/lvp_icd.x86_64.json VK_INSTANCE_LAYERS=VK_LAYER_KHRONOS_validation NPS64_DISABLE_TEXTURE_CACHE=1 BENCH_HLE_PREVIEW=1 BENCH_FRAMES=${BENCH_FRAMES:-330} BENCH_CPU=jit timeout 180 "$dir/frontend" > "$dir/uncached-host.log" 2>&1
python3 - <<'PY'
from pathlib import Path
import re
d=Path('build/hle-game-native')
cached=(d/'cached.log').read_text();uncached=Path('build/benchdata/nps64.log').read_text()
pattern=r'image_hash frame=(\d+) value=([0-9a-f]+)'
assert len(re.findall(pattern,cached))==64
assert re.findall(pattern,cached)==re.findall(pattern,uncached)
assert (d/'cached-first.ppm').read_bytes()==Path('build/bench-frame-999.ppm').read_bytes()
assert (d/'cached-last.ppm').read_bytes()==Path('build/bench-frame-1000.ppm').read_bytes()
stats=lambda s: [(int(a),int(b)) for a,b in re.findall(r'texels decoded=(\d+) cache_hits=(\d+)',s)]
a=stats(cached);b=stats(uncached)
assert sum(y for x,y in a)>0 and sum(y for x,y in b)==0
assert sum(x for x,y in a)<sum(x for x,y in b)
s=(d/'uncached-host.log').read_text();assert 'VUID-' not in s and 'Validation Error' not in s
print('PASS all 64 cached frame hashes match uncached reference; texture decodes:',sum(x for x,y in a),'vs',sum(x for x,y in b))
PY
cp build/benchdata/nps64.log "$dir/uncached.log"
cp "$dir/cached.log" build/benchdata/nps64.log
