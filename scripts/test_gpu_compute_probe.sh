#!/usr/bin/env bash
set -euo pipefail
cd /work
mkdir -p build/gpu-compute-probe
build/gpu/glslang-host/StandAlone/glslang -V --target-env vulkan1.1 tests/gpu_compute_probe.comp -o build/gpu-compute-probe/probe.spv
clang-21 -std=c11 -O2 -Wall -Wextra -Werror tests/gpu_compute_probe.c -lvulkan -o build/gpu-compute-probe/probe
VK_ICD_FILENAMES=/usr/share/vulkan/icd.d/lvp_icd.x86_64.json VK_INSTANCE_LAYERS=VK_LAYER_KHRONOS_validation timeout 30 build/gpu-compute-probe/probe build/gpu-compute-probe/probe.spv > build/gpu-compute-probe/results.log 2>&1
cat build/gpu-compute-probe/results.log
if grep -E 'Validation Error|VUID-' build/gpu-compute-probe/results.log; then exit 1; fi
