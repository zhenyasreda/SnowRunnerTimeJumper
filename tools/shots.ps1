# Screenshots for the documentation: runs the fake game and the tool with the
# requested theme/language and saves the window into a PNG file.
#
#   powershell -ExecutionPolicy Bypass -File tools\shots.ps1
#
# Requires MinGW-w64 (gcc) in PATH.

$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
Set-Location $root

$build = Join-Path $root 'build'
if (-not (Test-Path $build)) { New-Item -ItemType Directory -Path $build | Out-Null }

& gcc -O2 -std=gnu11 -o "$build\fake_game.exe" tests\fake_game.c tests\fake_game.S
if ($LASTEXITCODE -ne 0) { Write-Host 'Build failed' -ForegroundColor Red; exit 1 }
Copy-Item "$build\fake_game.exe" "$build\SnowRunner.exe" -Force

$logDir = Join-Path $env:LOCALAPPDATA 'SnowRunnerTimeJumper'
$settings = Join-Path $logDir 'settings.ini'
if (-not (Test-Path $logDir)) { New-Item -ItemType Directory -Path $logDir | Out-Null }

Add-Type -AssemblyName System.Drawing

function Save-Shot([string]$theme, [string]$lang, [string]$png) {
    Set-Content -Path $settings -Value "theme=$theme`nlang=$lang" -Encoding ASCII
    $game = Start-Process -FilePath "$build\SnowRunner.exe" -PassThru -WindowStyle Minimized
    Start-Sleep -Seconds 2
    $bmp = "$build\shot_$theme`_$lang.bmp"
    # The window must not be minimized: the snapshot needs a real window DC.
    $tool = Start-Process -FilePath "$build\SnowRunnerTimeJumper.exe" -PassThru `
                          -ArgumentList "--snapshot=`"$bmp`""
    $tool | Wait-Process -Timeout 30
    Stop-Process -Id $game.Id -Force -ErrorAction SilentlyContinue
    if (-not (Test-Path $bmp)) { throw "Snapshot was not created: $bmp" }
    $img = [System.Drawing.Image]::FromFile($bmp)
    $img.Save($png, [System.Drawing.Imaging.ImageFormat]::Png)
    $img.Dispose()
    Write-Host "Saved: $png"
}

Save-Shot 'dark'  'ru' "$root\docs\screenshot.png"
Copy-Item "$root\docs\screenshot.png" "$root\docs\media\screenshot-dark.png" -Force
Save-Shot 'light' 'ru' "$root\docs\media\screenshot-light.png"
Write-Host 'Done.'
