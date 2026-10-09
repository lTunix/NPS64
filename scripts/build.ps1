param([switch]$Tests, [int]$Jobs = 4)
$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot
Set-Location -LiteralPath $projectRoot
docker build -t nps64-builder:local .
if ($LASTEXITCODE -ne 0) { throw 'No se pudo construir la imagen Docker' }
$action = if ($Tests) { 'scripts/test.sh' } else { 'scripts/build.sh' }
docker run --rm --mount "type=bind,source=$projectRoot,target=/work" -w /work nps64-builder:local bash scripts/bootstrap.sh
if ($LASTEXITCODE -ne 0) { throw 'Fallo preparando fuentes o SDK' }
docker run --rm --mount "type=bind,source=$projectRoot,target=/work" -w /work -e "JOBS=$Jobs" nps64-builder:local bash $action
if ($LASTEXITCODE -ne 0) { throw "Fallo en $action; revisar la salida" }
