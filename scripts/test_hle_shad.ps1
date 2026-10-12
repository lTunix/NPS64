param(
    [string]$Emulator = 'C:\Users\marco\Downloads\NPS64\build\shadps4\runtime\shadPS4-surface-sync.exe',
    [string]$Label = 'hle-66',
    [int]$Seconds = 60,
    [switch]$EnterGame
)
$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$runtime = Join-Path $root 'build/shad-runtime'
$log = Join-Path $runtime 'user/data/NPS64/nps64.log'
$eboot = Join-Path $root 'build/pkg-verify/uroot/eboot.bin'
if (-not (Test-Path $eboot)) { throw 'Generar y verificar EmulatorPackage primero' }
if (Get-Process shadPS4-surface-sync -ErrorAction SilentlyContinue) { throw 'Hay otra prueba shadPS4 activa' }
Copy-Item -LiteralPath $Emulator -Destination "$runtime/shadPS4-surface-sync.exe" -Force
if (Test-Path $log) { Move-Item -LiteralPath $log -Destination "$root/build/$Label-previous.log" -Force }
Add-Type -TypeDefinition 'using System; using System.Runtime.InteropServices; public class HleBenchKeys { [DllImport("user32.dll")] public static extern bool PostMessage(IntPtr h,uint m,IntPtr w,IntPtr l); }'
$process = Start-Process "$runtime/shadPS4-surface-sync.exe" -ArgumentList @('-g', $eboot, '--config-global') -WorkingDirectory $runtime -WindowStyle Hidden -RedirectStandardOutput "$root/build/$Label-shad-stdout.log" -RedirectStandardError "$root/build/$Label-shad-stderr.log" -PassThru
function Keys([int[]]$codes) {
    $process.Refresh()
    foreach ($code in $codes) { [HleBenchKeys]::PostMessage($process.MainWindowHandle,0x100,[IntPtr]$code,[IntPtr]0) | Out-Null }
    Start-Sleep -Milliseconds 220
    foreach ($code in $codes) { [HleBenchKeys]::PostMessage($process.MainWindowHandle,0x101,[IntPtr]$code,[IntPtr]0) | Out-Null }
}
try {
    for ($i=0;$i -lt 60;$i++) {
        Start-Sleep -Milliseconds 500; $process.Refresh()
        if ($process.HasExited) { throw 'shadPS4 termino durante el arranque' }
        if ((Test-Path $log) -and (Select-String -LiteralPath $log -SimpleMatch 'state=library' -Quiet)) { break }
    }
    Keys @(0x4e)
    Start-Sleep -Seconds 12
    Keys @(0x58,0x4d)
    Start-Sleep -Milliseconds 500
    Keys @(0x43)
    Start-Sleep -Seconds 15
    if ($EnterGame) {
        Keys @(0x0d)
        Start-Sleep -Seconds 5
        Keys @(0x4e)
        Start-Sleep -Seconds 5
        Keys @(0x4e)
        Start-Sleep -Seconds 10
    }
    $start = (Get-Content -LiteralPath $log).Count
    for ($elapsed=0;$elapsed -lt $Seconds;$elapsed+=10) {
        Start-Sleep -Seconds ([Math]::Min(10,$Seconds-$elapsed))
        $process.Refresh()
        if ($process.HasExited) { throw 'shadPS4 termino durante Vulkan' }
        Write-Host "Vulkan: $([Math]::Min($elapsed+10,$Seconds))/$Seconds segundos"
    }
    Copy-Item -LiteralPath $log -Destination "$root/build/$Label-core.log" -Force
    $samples = Get-Content -LiteralPath $log | Select-Object -Skip $start
    $fps = @($samples | ForEach-Object { if ($_ -match 'state=game fps=([0-9.]+).*gpu_ms=([0-9.]+)') { [pscustomobject]@{fps=[double]::Parse($Matches[1],[cultureinfo]::InvariantCulture);gpu_ms=[double]::Parse($Matches[2],[cultureinfo]::InvariantCulture)} } })
    if ($fps.Count -lt 10 -or -not ($fps | Where-Object gpu_ms -gt 0)) { throw 'Faltan muestras GPU reales' }
    $all = Get-Content -LiteralPath $log -Raw
    if ($all -match '\[hle-live\] renderer failed|GPU hang\?') { throw 'Fallo de renderer/GPU' }
    $report = [ordered]@{ label=$Label; seconds=$Seconds; samples=$fps.Count; mean_fps=($fps.fps | Measure-Object -Average).Average; min_fps=($fps.fps | Measure-Object -Minimum).Minimum; mean_gpu_ms=($fps.gpu_ms | Measure-Object -Average).Average; samples_at_least_48=@($fps | Where-Object fps -ge 48).Count; cpu_reference_retained=$true; physical_ps4_fps=$false }
    $report | ConvertTo-Json | Set-Content "$root/build/$Label-performance.json"
    $report | ConvertTo-Json | Write-Host
} finally {
    $process.Refresh()
    if (-not $process.HasExited) { Stop-Process -Id $process.Id -Force; $process.WaitForExit(); Write-Host 'Prueba detenida por el runner; no crash' }
}
