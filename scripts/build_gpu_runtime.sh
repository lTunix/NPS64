#!/usr/bin/env bash
set -euo pipefail
cd /work
export PSB=/work/build/gpu OPENORBIS=/work/toolchain/OpenOrbis/PS4Toolchain
export LLVM_LOCAL_SRC=/work/vendor/gpu/llvm-project-21.1.6.src JOBS=${JOBS:-4}
bash build/gpu/dolphin-port/toolchain/build-libcxx.sh > build/gpu/libcxx-build.log 2>&1 || { tail -60 build/gpu/libcxx-build.log; exit 1; }
