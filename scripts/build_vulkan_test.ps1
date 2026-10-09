param([int]$Jobs = 4)
$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot
Set-Location -LiteralPath $projectRoot
function Invoke-TestBuildStep([string]$Script) {
    docker run --rm --mount "type=bind,source=$projectRoot,target=/work" -w /work -e "JOBS=$Jobs" -e NPS64_GPU_EXPERIMENTAL=1 -e NPS64_SOURCE_VERSION=00.19 nps64-gpu-builder:local bash $Script
    if ($LASTEXITCODE -ne 0) { throw "Fallo en $Script" }
}
# Explicit experimental entry; the standard build retains CPU/quarantined GPU.
Invoke-TestBuildStep 'scripts/bootstrap.sh'
Invoke-TestBuildStep 'scripts/bootstrap_gpu.sh'
docker run --rm --mount "type=bind,source=$projectRoot,target=/work" -w /work nps64-gpu-builder:local python3 scripts/compile_gpu_shaders.py
if ($LASTEXITCODE -ne 0) { throw 'Fallo compilando el shader de tiles independiente' }
docker run --rm --mount "type=bind,source=$projectRoot,target=/work" -w /work nps64-gpu-builder:local python3 scripts/patch_gpu_driver.py reference/DolphinPS4/port/mesa/ac_ps4_drm.c vendor/gpu/mesa-26.0.8/src/amd/common/ac_ps4_drm.c
if ($LASTEXITCODE -ne 0) { throw 'Fallo preparando RADV/GNM' }
foreach ($test in @('tests/test_gpu_driver_patch.py', 'tests/test_gpu_hang_capture.py')) {
    docker run --rm --mount "type=bind,source=$projectRoot,target=/work" -w /work nps64-gpu-builder:local python3 $test
    if ($LASTEXITCODE -ne 0) { throw "Fallo en $test" }
}
Invoke-TestBuildStep 'scripts/build_mesa_ps4.sh'
Invoke-TestBuildStep 'scripts/build_gpu.sh'
Invoke-TestBuildStep 'scripts/source_bundle.sh'
docker run --rm --mount "type=bind,source=$projectRoot,target=/work" -w /work nps64-gpu-builder:local python3 scripts/check_vulkan_test_artifacts.py
if ($LASTEXITCODE -ne 0) { throw 'Fallo verificando los artefactos Vulkan experimentales' }
