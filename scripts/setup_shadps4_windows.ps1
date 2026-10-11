$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
$base = Join-Path $root 'build/shadps4'
$source = Join-Path $base 'source'
$tools = Join-Path $base 'tools'
New-Item -ItemType Directory -Force -Path $tools | Out-Null
if (!(Test-Path "$source/.git")) {
    & git clone --depth 1 --branch v.0.19.0 https://github.com/shadps4-emu/shadPS4.git $source
    if ($LASTEXITCODE) { throw 'shadPS4 clone failed' }
}
if ((& git -C $source rev-parse HEAD) -ne 'c7e065d1b415be16c23e260a21f1dd8bbfc4cb57') {
    throw 'Existing source has a different revision; refusing to replace it'
}
& git -C $source submodule update --init --recursive --depth 1 --jobs 6
if ($LASTEXITCODE) { throw 'Submodule setup failed' }
$archive = Join-Path $tools 'LLVM-19.1.7-win64.exe'
if (!(Test-Path $archive)) {
    Invoke-WebRequest 'https://github.com/llvm/llvm-project/releases/download/llvmorg-19.1.7/LLVM-19.1.7-win64.exe' -OutFile $archive
}
$expected = 'f19ae5bc4823ac69ec01dc2ded503ec80a04ad2208dda1595d1f0413c148ef90'
if ((Get-FileHash $archive -Algorithm SHA256).Hash.ToLowerInvariant() -ne $expected) {
    throw 'LLVM archive checksum mismatch'
}
if (!(Test-Path "$tools/llvm/bin/clang-cl.exe")) {
    $sevenZip = Join-Path $env:ProgramFiles '7-Zip/7z.exe'
    if (!(Test-Path $sevenZip)) { throw 'Requires 7-Zip to extract portable LLVM' }
    & $sevenZip x $archive "-o$tools/llvm" -y
    if ($LASTEXITCODE) { throw 'LLVM extraction failed' }
}
Write-Host 'Pinned shadPS4 source and portable LLVM ready under build/shadps4'
