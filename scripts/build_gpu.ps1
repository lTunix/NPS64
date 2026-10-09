param([switch]$Tests, [int]$Jobs = 4)
$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot
Set-Location -LiteralPath $projectRoot
function Invoke-BuildStep([string]$Script) {
    docker run --rm --mount "type=bind,source=$projectRoot,target=/work" -w /work -e "JOBS=$Jobs" nps64-gpu-builder:local bash $Script
    if ($LASTEXITCODE -ne 0) { throw "Fallo en $Script; revisar build/gpu y build/core-gpu-build.log" }
}
docker build -t nps64-builder:local .
if ($LASTEXITCODE -ne 0) { throw 'Fallo construyendo Docker base' }
docker build -f Dockerfile.gpu -t nps64-gpu-builder:local .
if ($LASTEXITCODE -ne 0) { throw 'Fallo construyendo Docker GPU' }
Invoke-BuildStep 'scripts/bootstrap.sh'
Invoke-BuildStep 'scripts/bootstrap_gpu.sh'
docker run --rm --mount "type=bind,source=$projectRoot,target=/work" -w /work nps64-gpu-builder:local python3 scripts/compile_gpu_shaders.py
if ($LASTEXITCODE -ne 0) { throw 'Fallo compilando shader de tiles' }
if ($Tests) {
    Invoke-BuildStep 'scripts/test.sh'
    Invoke-BuildStep 'scripts/build_native_gpu.sh'
    Invoke-BuildStep 'scripts/test_gpu.sh'
    Invoke-BuildStep 'scripts/test_gpu_compute_probe.sh'
    docker run --rm --mount "type=bind,source=$projectRoot,target=/work" -w /work nps64-gpu-builder:local python3 tests/test_gpu_tile_equivalence.py
    if ($LASTEXITCODE -ne 0) { throw 'Fallo comparando los shaders de tiles' }
    Invoke-BuildStep 'scripts/test_gpu_heap.sh'
    Invoke-BuildStep 'scripts/test_gpu_platform.sh'
    Invoke-BuildStep 'scripts/test_gpu_quarantine.sh'
    docker run --rm --mount "type=bind,source=$projectRoot,target=/work" -w /work nps64-gpu-builder:local python3 tests/test_gpu_driver_patch.py
    if ($LASTEXITCODE -ne 0) { throw 'Fallo en pruebas del parche del driver GPU' }
} else {
    Invoke-BuildStep 'scripts/build_gpu_runtime.sh'
    Invoke-BuildStep 'scripts/build_mesa_ps4.sh'
    Invoke-BuildStep 'scripts/build_gpu.sh'
    Invoke-BuildStep 'scripts/source_bundle.sh'
}
