#!/usr/bin/env bash
set -euo pipefail
cd /work
mkdir -p build
python3 - <<'PY'
from PIL import Image
Image.new('RGB',(32,16),(224,48,32)).save('build/test-cover.png')
PY
clang -std=c11 -O1 -g -fsanitize=address,undefined -Wall -Wextra -Werror -Isrc tests/test_cover_match.c src/cover_match.c -o build/test-cover-match
build/test-cover-match
clang -std=c11 -D_DEFAULT_SOURCE -D_POSIX_C_SOURCE=200809L -DCOVER_ROOT='"/tmp/nps64-covers-test"' -O1 -g -fsanitize=address,undefined -Wall -Wextra -Werror -Wno-unused-function -Isrc tests/test_covers.c src/cover_match.c src/storage.c -lpthread -lm -o build/test-covers
build/test-covers
