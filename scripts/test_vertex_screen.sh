#!/usr/bin/env bash
set -euo pipefail
cd /work
core=${1:-build/native-gpu-asan}
emit="$core/mupen64plus-video-angrylion"
clang-21 -std=c11 -O2 -g -fsanitize=address,undefined -DNPS64_SCREEN_ORACLE \
    -I"$emit" -c "$emit/rdp_emit_bridge.c" -o build/screen-oracle-bridge.o
clang-21 -std=c11 -O2 -g -fsanitize=address,undefined -I"$emit" \
    tests/test_vertex_screen.c build/screen-oracle-bridge.o \
    "$emit/rdp_emit_frontend.o" "$emit/rdp_emit_rsp.o" "$emit/rdp_emit.o" \
    -lm -o build/test-vertex-screen
build/test-vertex-screen
