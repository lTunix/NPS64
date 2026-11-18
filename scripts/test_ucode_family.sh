#!/usr/bin/env bash
set -euo pipefail
cd /work
core=${1:-build/native-gpu-asan}
mkdir -p build/ucode-family-test
python3 - "$core" <<'PY'
from pathlib import Path
import sys
s=(Path(sys.argv[1])/'mupen64plus-video-angrylion/rdp_emit_hle.c').read_text()
a=s.rindex('static int f3d_ucode_family(')
b=s.index('\n}\n',a)+3
Path('build/ucode-family-test/ucode_family_generated.h').write_text(s[a:b])
PY
clang-21 -std=c11 -O1 -g -fsanitize=address,undefined -Wall -Wextra -Werror \
    -Ibuild/ucode-family-test tests/test_ucode_family.c -o build/test-ucode-family
build/test-ucode-family
