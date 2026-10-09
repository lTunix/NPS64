param([int]$Jobs = 4)
$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot
Set-Location -LiteralPath $projectRoot
docker run --rm --mount "type=bind,source=$projectRoot,target=/work" -w /work -e "JOBS=$Jobs" nps64-gpu-builder:local bash scripts/build_parallel_game.sh
if ($LASTEXITCODE -ne 0) { throw 'Fallo construyendo ParaLLEl-RDP PS4' }
