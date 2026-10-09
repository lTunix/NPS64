#!/usr/bin/env bash
set -euo pipefail
cd /work
export PATH=/work/build/gpu/host-tools/bin:$PATH
export CC=clang-21 CXX=clang++-21
prefix=/work/build/radv-native-deps
if [ ! -f build/libdrm-native/build.ninja ]; then
    meson setup build/libdrm-native vendor/gpu/libdrm-2.4.125 --prefix="$prefix" --libdir=lib \
      -Dintel=disabled -Dradeon=disabled -Damdgpu=enabled -Dnouveau=disabled \
      -Dvmwgfx=disabled -Dfreedreno=disabled -Dvc4=disabled -Detnaviv=disabled \
      -Dtests=false -Dinstall-test-programs=false
fi
ninja -C build/libdrm-native -j4
meson install -C build/libdrm-native
export PKG_CONFIG_PATH="$prefix/lib/pkgconfig"
if [ ! -f build/radv-native/build.ninja ]; then
    meson setup build/radv-native vendor/gpu/mesa-26.0.8 \
      -Dvulkan-drivers=amd -Dgallium-drivers= -Dplatforms= -Dllvm=disabled \
      -Damd-use-llvm=false -Dopengl=false -Dgles1=disabled -Dgles2=disabled -Dglx=disabled \
      -Degl=disabled -Dgbm=disabled -Dzstd=disabled -Dexpat=disabled -Dxmlconfig=disabled \
      -Dvalgrind=disabled -Dlibunwind=disabled -Dlmsensors=disabled -Dbuild-tests=false \
      -Dzlib=disabled -Dshader-cache=disabled -Dvulkan-layers= -Dtools=drm-shim \
      -Dvideo-codecs= -Dspirv-tools=disabled -Dperfetto=false -Dselinux=false
fi
ninja -C build/radv-native -j4 src/amd/vulkan/libvulkan_radeon.so \
  src/amd/drm-shim/libamdgpu_noop_drm_shim.so
