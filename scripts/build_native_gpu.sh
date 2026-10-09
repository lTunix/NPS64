#!/usr/bin/env bash
set -euo pipefail
cd /work
core_dir="${NATIVE_GPU_DIR:-build/native-gpu}"
mkdir -p "$core_dir"
tar -C vendor/parallel-n64 --exclude=.git -cf - . | tar -C "$core_dir" -xf -
python3 scripts/patch_core.py "$core_dir"
args=(platform=unix ARCH=x86_64 WITH_DYNAREC=x86_64 WANT_HACKTARUX=0 HAVE_ARI64_X64=1 HAVE_OPENGL=0 HAVE_PARALLEL=1 HAVE_PARALLEL_RSP=0 HAVE_THR_AL=1 STATIC_LINKING=1 TARGET=parallel_n64.a "CPUOPTS=${CORE_OPT:--O2} -DNDEBUG -DNPS64_COMPRESSED_MEMORY" GIT_VERSION=890323d CC=clang-21 CXX=clang++-21 AR=llvm-ar-21)
make -C "$core_dir" asm-defines-regen "${args[@]}" > "$core_dir-build.log" 2>&1
make -C "$core_dir" -j"${JOBS:-4}" "${args[@]}" >> "$core_dir-build.log" 2>&1 || { tail -80 "$core_dir-build.log"; exit 1; }
