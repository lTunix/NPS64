#!/usr/bin/env bash
set -euo pipefail
cd /work
mkdir -p downloads vendor/gpu build/gpu
fetch() {
    test -s "downloads/$2" || curl -fL --retry 3 "$1" -o "downloads/$2"
    python3 - "$2" <<'PY'
import hashlib,json,sys
from pathlib import Path
name=sys.argv[1];entry=json.loads(Path('sources.lock.json').read_text())[name]
assert hashlib.sha256(Path(entry['path']).read_bytes()).hexdigest()==entry['sha256'], 'Checksum mismatch: '+name
PY
}
fetch https://archive.mesa3d.org/mesa-26.0.8.tar.xz mesa-26.0.8.tar.xz
fetch https://dri.freedesktop.org/libdrm/libdrm-2.4.125.tar.xz libdrm-2.4.125.tar.xz
fetch https://github.com/llvm/llvm-project/releases/download/llvmorg-21.1.6/llvm-project-21.1.6.src.tar.xz llvm-project-21.1.6.src.tar.xz
test -d vendor/gpu/mesa-26.0.8 || tar -xJf downloads/mesa-26.0.8.tar.xz -C vendor/gpu
test -d vendor/gpu/libdrm-2.4.125 || tar -xJf downloads/libdrm-2.4.125.tar.xz -C vendor/gpu
if [ ! -d vendor/gpu/llvm-project-21.1.6.src/libcxx ]; then
    tar -xJf downloads/llvm-project-21.1.6.src.tar.xz -C vendor/gpu \
        llvm-project-21.1.6.src/{runtimes,libcxx,libcxxabi,libunwind,libc,cmake,llvm/cmake,llvm/utils/lit,llvm/utils/llvm-lit,third-party}
fi
sha256sum downloads/mesa-26.0.8.tar.xz downloads/libdrm-2.4.125.tar.xz downloads/llvm-project-21.1.6.src.tar.xz > build/gpu/download-sha256.log
if [ ! -f vendor/gpu/mesa-26.0.8/.nps64-patched ]; then
    python3 -c "from pathlib import Path;Path('build/gpu/mesa-ps4.patch').write_bytes(Path('reference/DolphinPS4/patches/mesa-ps4.patch').read_bytes().replace(b'\\r\\n',b'\\n'))"
    git -C vendor/gpu/mesa-26.0.8 apply /work/build/gpu/mesa-ps4.patch
    touch vendor/gpu/mesa-26.0.8/.nps64-patched
fi

python3 - <<'PY'
from pathlib import Path
import shutil
src=Path('reference/DolphinPS4');dst=Path('build/gpu/dolphin-port')
for sub in ['toolchain','port','scripts']:
 shutil.copytree(src/sub,dst/sub,dirs_exist_ok=True)
for p in dst.rglob('*'):
 if p.is_file() and p.suffix in {'.sh','.cmake','.c','.cpp','.h','.S','.x'}:
  p.write_bytes(p.read_bytes().replace(b'\r\n',b'\n'))
  if p.suffix=='.sh':p.chmod(0o755)
PY

# Mesa 26 requires glslang >=12.2; Ubuntu 22.04 ships 11.
fetch https://codeload.github.com/KhronosGroup/glslang/tar.gz/refs/tags/15.1.0 glslang-15.1.0.tar.gz
if [ ! -d vendor/gpu/glslang-15.1.0 ]; then tar -xzf downloads/glslang-15.1.0.tar.gz -C vendor/gpu; fi
cmake -S vendor/gpu/glslang-15.1.0 -B build/gpu/glslang-host -G Ninja -DCMAKE_BUILD_TYPE=Release -DENABLE_OPT=OFF -DENABLE_HLSL=OFF -DBUILD_TESTING=OFF -DGLSLANG_TESTS=OFF -DCMAKE_INSTALL_PREFIX=/work/build/gpu/host-tools
cmake --build build/gpu/glslang-host -j "${JOBS:-4}"
cmake --install build/gpu/glslang-host
sha256sum downloads/glslang-15.1.0.tar.gz >> build/gpu/download-sha256.log

python3 - <<'PY'
import hashlib,json
from pathlib import Path
for name,entry in json.loads(Path('sources.lock.json').read_text()).items():
 if 'sha256' in entry:
  assert hashlib.sha256(Path(entry['path']).read_bytes()).hexdigest()==entry['sha256'], 'Checksum mismatch: '+name
print('GPU dependency checksums match sources.lock.json')
PY
