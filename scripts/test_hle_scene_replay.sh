#!/usr/bin/env bash
set -euo pipefail
cd /work
dir="${1:-build/hle-screen-space}"
clang -std=c11 -O2 -Wall -Wextra -Werror -Isrc src/gpu_hle_probe.c src/hle_display_list.c tests/hle_scene_replay.c -lvulkan -lm -o "$dir/replay"
for scanout in "$dir"/frame-*-scanout.bin; do
  prefix="${scanout%-scanout.bin}"
  VK_ICD_FILENAMES=/usr/share/vulkan/icd.d/lvp_icd.x86_64.json VK_INSTANCE_LAYERS=VK_LAYER_KHRONOS_validation "$dir/replay" "$prefix" "$prefix-replay.ppm" > "$prefix-replay.log" 2>&1
done
python3 - "$dir" <<'PY'
from pathlib import Path
from PIL import Image
import json,sys
d=Path(sys.argv[1]);frames=list(d.glob('frame-*-scanout.bin'));assert frames
for f in frames:
    prefix=str(f)[:-len('-scanout.bin')]
    raw=f.read_bytes();expected=b''.join(raw[i:i+3] for i in range(0,len(raw),4))
    assert Image.open(prefix+'-replay.ppm').tobytes()==expected,f
    meta=json.loads(Path(prefix+'.json').read_text())
    assert meta['scene_frame']<=meta['core_frame'] and meta['display_target']<=meta['vi_origin']
    log=Path(prefix+'-replay.log').read_text();assert 'VUID-' not in log and 'Validation Error' not in log
print('PASS VI-selected scene replay: exact GPU pixels for',len(frames),'captures')
PY
