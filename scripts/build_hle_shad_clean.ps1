$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$dependencies = Join-Path (Split-Path -Parent $root) 'NPS64'
$source = Join-Path $dependencies 'build/shadps4/source'
$file = Join-Path $source 'src/video_core/renderer_vulkan/vk_rasterizer.cpp'
$backup = [IO.File]::ReadAllBytes($file)
$temporary = Join-Path $root 'build/build-shad-clean.ps1'
try {
    $env:NPS64_HLE_SHAD_SOURCE = $source
    @'
import os, subprocess
from pathlib import Path
source=Path(os.environ['NPS64_HLE_SHAD_SOURCE'])
path='src/video_core/renderer_vulkan/vk_rasterizer.cpp'
data=subprocess.check_output(['git','-C',str(source),'show','c7e065d1b415be16c23e260a21f1dd8bbfc4cb57:'+path])
(source/path).write_bytes(data)
'@ | python -
    if ($LASTEXITCODE) { throw 'No se pudo restaurar rasterizer base' }
    & python "$root/scripts/patch_shadps4_scanout_readback.py" $source
    if ($LASTEXITCODE) { throw 'Fallo readback GPU real' }
    $script = Get-Content "$dependencies/scripts/build_shadps4_windows.ps1" -Raw
    $script = $script.Replace('$root = Split-Path $PSScriptRoot -Parent', ('$root = ' + "'$dependencies'"))
    $script = $script.Replace('"$root/build/shadps4/runtime/shadPS4-surface-sync.exe"', "'$root/build/shad-clean.exe'")
    [IO.File]::WriteAllText($temporary, $script)
    & $temporary -SkipConfigure -Jobs 4
    if ($LASTEXITCODE) { throw 'Fallo compilando shadPS4 sin capturas diagnosticas' }
} finally {
    [IO.File]::WriteAllBytes($file, $backup)
    Remove-Item Env:NPS64_HLE_SHAD_SOURCE -ErrorAction SilentlyContinue
}
