param(
    [string]$Eboot = 'build/parallel-game/stage/eboot.bin',
    [string]$Rom = ''
)
$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
$runtime = Join-Path $root 'build/shadps4/runtime'
$archive = Join-Path $root 'build/shadps4/shadps4-win64-0.19.0.zip'
New-Item -ItemType Directory -Force -Path "$runtime/user" | Out-Null
if (!(Test-Path -LiteralPath $archive)) {
    Invoke-WebRequest 'https://github.com/shadps4-emu/shadPS4/releases/download/v.0.19.0/shadps4-win64-sdl-0.19.0.zip' -OutFile $archive
}
$expected = 'b9600602a809b7cb4aaf29cbeb0917d32caecf27bc5a9232df76200fbf80c017'
if ((Get-FileHash $archive -Algorithm SHA256).Hash.ToLowerInvariant() -ne $expected) {
    throw 'Release checksum mismatch'
}
if (!(Test-Path "$runtime/shadPS4.exe")) {
    Expand-Archive -LiteralPath $archive -DestinationPath $runtime -Force
}
$binary = (Resolve-Path -LiteralPath (Join-Path $root $Eboot)).Path
if ($Rom) {
    $source = (Resolve-Path -LiteralPath $Rom).Path
    New-Item -ItemType Directory -Force -Path "$runtime/user/data/NPS64/games" | Out-Null
    Copy-Item -LiteralPath $source -Destination "$runtime/user/data/NPS64/games/mario.n64"
}
# Isolated emulator profile: avoid Ari64's unsupported mprotect operation.
$diagnostics = "$runtime/user/data/NPS64-Parallel"
New-Item -ItemType Directory -Force -Path $diagnostics | Out-Null
Set-Content -LiteralPath "$diagnostics/diagnostics.cfg" -Value 'NPS64DIAG5 6 0 0 1 1' -Encoding ASCII
Write-Host 'Interactive test: select the ROM with N (Cross). Logs: build/shadps4/runtime/user'
Write-Host 'CPU fallback is not evidence that ParaLLEl RDP or PS4 GPU synchronization passes.'
Push-Location $runtime
try { & "$runtime/shadPS4.exe" -g $binary --config-clean }
finally { Pop-Location }
