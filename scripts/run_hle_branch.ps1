param(
    [ValidateSet('Compare', 'Fallback', 'Package', 'EmulatorPackage')][string]$Action = 'Compare',
    [string]$Dependencies = (Join-Path (Split-Path -Parent (Split-Path -Parent $PSScriptRoot)) 'NPS64'),
    [string]$Rom = 'Super Mario 64 (Europe) (En,Fr,De).n64'
)
$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$dependenciesPath = (Resolve-Path -LiteralPath $Dependencies).Path
New-Item -ItemType Directory -Force (Join-Path $root 'build') | Out-Null
$arguments = @('run', '--rm', '--mount', "type=bind,source=$root,target=/work", '-w', '/work')
foreach ($directory in @('vendor', 'toolchain', 'roms', 'build/native-jit', 'build/core-gpu', 'build/gpu')) {
    $source = Join-Path $dependenciesPath $directory
    if (-not (Test-Path -LiteralPath $source)) { throw "Falta dependencia: $source" }
    $arguments += @('--mount', "type=bind,source=$source,target=/work/$directory,readonly")
}
$arguments += 'nps64-gpu-builder:local'
if ($Action -eq 'EmulatorPackage') {
    $arguments = $arguments[0..($arguments.Length - 2)] + @('--mount', "type=bind,source=$dependenciesPath/build/shadps4/probe,target=/work/build/shad-adapters,readonly", 'nps64-gpu-builder:local')
}
switch ($Action) {
    'Compare' { $arguments += @('bash', 'scripts/test_hle_compare.sh', "/work/roms/$Rom", 'build/hle-restored-comparison') }
    'Fallback' { $arguments += @('bash', 'scripts/test_hle_deferred_vi.sh', "/work/roms/$Rom") }
    'Package' { $arguments += @('bash', 'scripts/build_hle_game.sh') }
    'EmulatorPackage' { $arguments += @('python3', 'scripts/build_hle_shad.py') }
}
& docker @arguments
if ($LASTEXITCODE -ne 0) { throw "Fallo HLE: $Action; revisar build en $root" }
