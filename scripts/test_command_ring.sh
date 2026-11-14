#!/usr/bin/env bash
set -euo pipefail
cd /work
python3 - <<'PY'
from pathlib import Path
import shutil
out=Path('build/command-ring-unit');out.mkdir(exist_ok=True)
rel=Path('mupen64plus-video-paraLLEl/parallel-rdp/parallel-rdp')
shutil.copyfile(Path('vendor/parallel-n64')/rel/'command_ring.hpp',out/'command_ring.hpp')
(out/'rdp_device.hpp').write_text('#pragma once\n#include <stdint.h>\nnamespace RDP {enum class Op:unsigned {MetaIdle=1};class CommandProcessor{public:unsigned expected=0;void enqueue_command_direct(unsigned,const uint32_t*);};}\n')
(out/'thread_id.hpp').write_text('#pragma once\nnamespace Util {inline void register_thread_index(unsigned){}}\n')
for name,root in [('baseline','vendor/parallel-n64'),('optimized','build/native-gpu-asan')]:shutil.copyfile(Path(root)/rel/'command_ring.cpp',out/(name+'.cpp'))
PY
flags=(-O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer -Wall -Wextra -Werror -Ibuild/command-ring-unit -Ivendor/parallel-n64/libretro-common/include)
clang -std=c11 "${flags[@]}" -c vendor/parallel-n64/libretro-common/rthreads/retro_eventcount.c -o build/command-ring-unit/eventcount.o
clang -std=c11 -D_GNU_SOURCE "${flags[@]}" -c vendor/parallel-n64/libretro-common/rthreads/rthreads.c -o build/command-ring-unit/rthreads.o
for mode in baseline optimized; do
 clang++-21 -resource-dir=/work/build/tools/usr/lib/llvm-18/lib/clang/18 -std=c++11 "${flags[@]}" tests/test_command_ring.cpp "build/command-ring-unit/$mode.cpp" build/command-ring-unit/eventcount.o build/command-ring-unit/rthreads.o -lpthread -o "build/command-ring-unit/$mode"
 timeout 25 "build/command-ring-unit/$mode"
done
