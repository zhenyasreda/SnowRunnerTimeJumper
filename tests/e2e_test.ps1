# End-to-end test: builds a fake "game" with the very same clock-update
# instruction as SnowRunner, runs it as SnowRunner.exe, starts the tool and
# checks from the log that it connected to the game.
#
#   powershell -ExecutionPolicy Bypass -File tests\e2e_test.ps1
#
# Requires MinGW-w64 (gcc) in PATH.

$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
Set-Location $root

$build = Join-Path $root 'build'
if (-not (Test-Path $build)) { New-Item -ItemType Directory -Path $build | Out-Null }

Write-Host '[1/4] Building the fake game...' -ForegroundColor Cyan
& gcc -O2 -std=gnu11 -o "$build\fake_game.exe" tests\fake_game.c tests\fake_game.S
if ($LASTEXITCODE -ne 0) { Write-Host 'Build failed' -ForegroundColor Red; exit 1 }
Copy-Item "$build\fake_game.exe" "$build\SnowRunner.exe" -Force

$logDir = Join-Path $env:LOCALAPPDATA 'SnowRunnerTimeJumper'
$logFile = Join-Path $logDir 'srtc.log'
if (Test-Path $logFile) { Remove-Item $logFile -Force }

Write-Host '[2/4] Starting the fake game...' -ForegroundColor Cyan
$game = Start-Process -FilePath "$build\SnowRunner.exe" -PassThru -WindowStyle Minimized
Start-Sleep -Seconds 2

Write-Host '[3/4] Starting the tool...' -ForegroundColor Cyan
$tool = Start-Process -FilePath "$build\SnowRunnerTimeJumper.exe" -PassThru -WindowStyle Minimized
Start-Sleep -Seconds 5

Write-Host '[4/4] Checking the log...' -ForegroundColor Cyan
$attached = $false
if (Test-Path $logFile) {
    Get-Content $logFile -Encoding UTF8 | ForEach-Object { Write-Host "    $_" }
    $text = Get-Content $logFile -Raw -Encoding UTF8
    $attached = ($text -match 'attach: OK')
} else {
    Write-Host '    LOG NOT FOUND' -ForegroundColor Red
}

Stop-Process -Id $tool.Id -Force -ErrorAction SilentlyContinue
Start-Sleep -Seconds 1
Stop-Process -Id $game.Id -Force -ErrorAction SilentlyContinue

if ($attached) {
    Write-Host ''
    Write-Host 'Test passed: the tool connected to the game.' -ForegroundColor Green
    exit 0
}
Write-Host ''
Write-Host 'Test failed: no connection detected.' -ForegroundColor Red
exit 1
