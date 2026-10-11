param([int]$Jobs = 4)
$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
$source = Join-Path $root 'build/shadps4/source'
$build = Join-Path $root 'build/shadps4/native-build'
$llvm = Join-Path $root 'build/shadps4/tools/llvm/bin'
$vswhere = "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe"
$vs = & $vswhere -latest -products '*' -property installationPath
if (!$vs -or !(Test-Path "$llvm/clang-cl.exe")) {
    throw 'Requires Visual Studio C++ tools and portable LLVM under build/shadps4/tools/llvm'
}
$revision = & git -C $source rev-parse HEAD
if ($revision -ne 'c7e065d1b415be16c23e260a21f1dd8bbfc4cb57') {
    throw 'Requires shadPS4 v0.19.0 source and recursive submodules'
}
& python "$root/scripts/patch_shadps4_surface_sync.py" $source
if ($LASTEXITCODE) { throw 'Patch failed' }
$devcmd = Join-Path $vs 'Common7/Tools/VsDevCmd.bat'
# Capture the environment locally without printing credentials or environment values.
$buildEnvironment = & cmd.exe /d /s /c "`"$devcmd`" -arch=x64 -host_arch=x64 >nul && set"
if ($LASTEXITCODE) { throw 'Visual Studio environment initialization failed' }
foreach ($entry in $buildEnvironment) {
    if ($entry -match '^([^=]+)=(.*)$') {
        [Environment]::SetEnvironmentVariable($matches[1], $matches[2], 'Process')
    }
}
$env:PATH = "$llvm;$vs/Common7/IDE/CommonExtensions/Microsoft/CMake/Ninja;$env:PATH"
Push-Location $source
try {
    & cmake -S $source -B $build -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER=clang-cl -DCMAKE_CXX_COMPILER=clang-cl -DENABLE_DISCORD_RPC=OFF -DENABLE_UPDATER=OFF
    if ($LASTEXITCODE) { throw 'CMake configure failed' }
    & cmake --build $build --parallel $Jobs
    if ($LASTEXITCODE) { throw 'Emulator build failed' }
}
finally { Pop-Location }
New-Item -ItemType Directory -Force -Path "$root/build/shadps4/runtime" | Out-Null
Copy-Item -LiteralPath "$build/shadPS4.exe" -Destination "$root/build/shadps4/runtime/shadPS4-surface-sync.exe"
Write-Host 'Built isolated shadPS4-surface-sync.exe; official executable unchanged'
