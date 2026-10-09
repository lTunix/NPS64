#!/usr/bin/env bash
set -euo pipefail
cd /work
rom="${1:?Pass the local ROM path}"
dir=build/hle-deferred-vi
mkdir -p "$dir"
BENCH_HLE_LIVE=1 BENCH_SM64=1 BENCH_FRAMES=2600 NPS64_HLE_FRAME_HASHES=1 NPS64_TEST_HLE_FALLBACK_FRAME=2839 bash scripts/test_hle_game.sh "$rom"
cp build/benchdata/nps64.log "$dir/deferred-core.log"
cp build/hle-game-native/host.log "$dir/deferred-host.log"
: > build/benchdata/nps64.log
VK_ICD_FILENAMES=/usr/share/vulkan/icd.d/lvp_icd.x86_64.json VK_INSTANCE_LAYERS=VK_LAYER_KHRONOS_validation BENCH_HLE_PREVIEW=1 BENCH_HLE_LIVE=1 BENCH_SM64=1 BENCH_FRAMES=2600 BENCH_CPU=jit NPS64_HLE_FRAME_HASHES=1 NPS64_TEST_HLE_FALLBACK_FRAME=2839 NPS64_HLE_CPU_VI=1 timeout 180 build/hle-game-native/frontend > "$dir/reference-host.log" 2>&1
cp build/benchdata/nps64.log "$dir/reference-core.log"
python3 - <<'PY'
from pathlib import Path
import re
p=Path('build/hle-deferred-vi')
a=(p/'deferred-core.log').read_text();b=(p/'reference-core.log').read_text()
pattern=r'\[hle-hash\] frame=(\d+) GPU=(\d) bytes=(\d+) value=([0-9a-f]+)'
x=re.findall(pattern,a);y=re.findall(pattern,b)
assert len(x)>1000 and x==y,next(((i,u,v) for i,(u,v) in enumerate(zip(x,y)) if u!=v), (len(x),len(y)))
assert any(f=='2839' and gpu=='0' for f,gpu,size,h in x)
assert '[hle-live] test CPU fallback frame=2839' in a and '[hle-live] test CPU fallback frame=2839' in b
stats=[(int(d),int(r)) for d,r in re.findall(r'CPU VI deferred=(\d+) fallback_resolved=(\d+)',a)]
assert stats and max(d for d,r in stats)>500 and max(r for d,r in stats)>0
assert 'CPU VI deferred=' not in b
for mode in ('deferred','reference'):
    s=(p/(mode+'-host.log')).read_text();assert 'VUID-' not in s and 'Validation Error' not in s
    assert int(re.search(r'BENCH live_audio_frames=(\d+)',s)[1])>100000
print('PASS deferred VI:',len(x),'frame hashes match full CPU VI, including forced fallback, transitions, save/load and ROM reload')
PY
