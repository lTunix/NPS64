#!/usr/bin/env bash
set -euo pipefail
cd /work
core=${1:-build/native-opt-57}
emit="$core/mupen64plus-video-angrylion"
clang-21 -std=c11 -O2 -g -fsanitize=address,undefined -DNPS64_RSP_DIV_ORACLE \
 -I"$emit" -c "$emit/rdp_emit_rsp.c" -o build/rsp-div-oracle.o
clang-21 -std=c11 -O2 -g -fsanitize=address,undefined -I"$emit" \
 tests/test_rsp_div_normalize.c build/rsp-div-oracle.o -lm -o build/test-rsp-div-normalize
build/test-rsp-div-normalize
