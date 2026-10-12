#!/usr/bin/env bash
set -euo pipefail
cd /work
export OO_PS4_TOOLCHAIN="${OO_PS4_TOOLCHAIN:-/work/toolchain/OpenOrbis/PS4Toolchain}"
sdk="$OO_PS4_TOOLCHAIN"
rdp_profile_flags="${NPS64_RDP_PROFILE_FLAGS:--fprofile-instr-use=/work/assets/cpu-rdp-games.profdata -Werror=profile-instr-out-of-date}"
test -f assets/cpu-rdp-games.profdata || { echo "Falta perfil RDP: ejecutar scripts/train_cpu_games.sh con ROMs locales"; exit 1; }
test -f "$sdk/lib/libc.a" || { echo 'Falta SDK: ejecutar bash scripts/bootstrap.sh'; exit 1; }
mkdir -p build/core-jit build/stage/sce_sys out
# Keep cross objects separate from native test objects and upstream checkout.
tar -C vendor/parallel-n64 --exclude=.git -cf - . | tar -C build/core-jit -xf -
python3 scripts/patch_core.py build/core-jit
chmod +x scripts/ps4-cc scripts/ps4-cxx "$sdk/bin/linux/"*
make -C build/core-jit asm-defines-regen "NPS64_RDP_PROFILE_FLAGS=$rdp_profile_flags" platform=unix ARCH=x86_64 WITH_DYNAREC=x86_64 WANT_HACKTARUX=0 HAVE_ARI64_X64=1 HAVE_OPENGL=0 HAVE_PARALLEL=0 HAVE_PARALLEL_RSP=0 HAVE_THR_AL=1 STATIC_LINKING=1 TARGET=parallel_n64.a CPUOPTS='-O2 -DNDEBUG' GIT_VERSION=890323d CC=/work/scripts/ps4-cc CXX=/work/scripts/ps4-cxx AR=llvm-ar > build/core-jit-build.log 2>&1 || { tail -80 build/core-jit-build.log; exit 1; }
make -C build/core-jit -j"${JOBS:-4}" "NPS64_RDP_PROFILE_FLAGS=$rdp_profile_flags" platform=unix ARCH=x86_64 WITH_DYNAREC=x86_64 WANT_HACKTARUX=0 HAVE_ARI64_X64=1 HAVE_OPENGL=0 HAVE_PARALLEL=0 HAVE_PARALLEL_RSP=0 HAVE_THR_AL=1 STATIC_LINKING=1 TARGET=parallel_n64.a CPUOPTS='-O2 -DNDEBUG' GIT_VERSION=890323d CC=/work/scripts/ps4-cc CXX=/work/scripts/ps4-cxx AR=llvm-ar > build/core-jit-build.log 2>&1 || { tail -80 build/core-jit-build.log; exit 1; }
scripts/ps4-cc -std=c11 -O2 -Wall -Wextra -Ivendor/parallel-n64/libretro-common/include -Isrc -DNPS64_CPU_RDP_PGO -c src/frontend.c -o build/frontend.o
scripts/ps4-cc -std=c11 -O2 -Wall -Wextra -Isrc -c src/platform_ps4.c -o build/platform.o
scripts/ps4-cc -std=c11 -O2 -Wall -Wextra -Isrc -c src/storage.c -o build/storage.o
scripts/ps4-cc -std=c11 -O2 -Wall -Wextra -c src/third_party/ps4_malloc.c -o build/malloc.o
scripts/ps4-cc -std=c11 -O2 -Wall -Wextra -c src/runtime_ps4.c -o build/runtime.o
scripts/ps4-cc -std=c11 -O2 -Wall -Wextra -Isrc -c src/log.c -o build/log.o
ld.lld -m elf_x86_64 -pie -z max-page-size=0x4000 --script scripts/ps4-link.x --eh-frame-hdr --wrap=pthread_once --wrap=mprotect -L"$sdk/lib" "$sdk/lib/crt1.o" build/frontend.o build/log.o build/platform.o build/storage.o build/malloc.o build/runtime.o build/core-jit/parallel_n64.a -lc -lm -lkernel -lSceLibcInternal -lSceVideoOut -lScePad -lSceUserService -lSceAudioOut -lSceSystemService -lSceSysmodule -o build/nps64.elf
"$sdk/bin/linux/create-fself" -in=build/nps64.elf -out=build/nps64.oelf --eboot build/stage/eboot.bin --paid 0x3800000000000011
python3 scripts/check_elf.py
python3 scripts/assets.py
NPS64_VERSION=00.67 bash scripts/package.sh
bash scripts/verify_pkg.sh
cp out/IV0000-NPSX00064_00-NPS64PS400000000.pkg out/NPS64-by-lTunix-00.67.pkg
(cd out && sha256sum NPS64-by-lTunix-00.67.pkg > NPS64-by-lTunix-00.67-SHA256.txt)
echo 'PKG construido. La ejecucion y el rendimiento requieren pruebas en PS4 real.'
