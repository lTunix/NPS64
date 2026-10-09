#!/usr/bin/env bash
set -euo pipefail
cd /work
core=${1:?Pass a patched core source directory}
mkdir -p build/cpu-lane-check
python3 - "$core" <<'PY'
from pathlib import Path
import sys
text=(Path(sys.argv[1])/'mupen64plus-video-angrylion/n64video/rdp/rasterizer.c').read_text()
text=text[text.index('static STRICTINLINE uint32_t nps64_lane_remainder('):]
Path('build/cpu-lane-check/cpu_lane_generated.h').write_text(text[:text.index('\n}\n\n')+4])
PY
clang -O2 -g -fsanitize=address,undefined -Wall -Wextra -Werror \
  -Ibuild/cpu-lane-check tests/test_cpu_lane.c -o build/cpu-lane-check/test
build/cpu-lane-check/test
