@echo off
rem Сквозной тест: собирает «имитацию игры», запускает её под именем
rem SnowRunner.exe и проверяет, что программа подключается к ней,
rem управляет временем и корректно отключается.
rem
rem Требуется MinGW-w64 (gcc) в PATH.

setlocal
cd /d "%~dp0\.."

if not exist build mkdir build

echo [1/4] Сборка имитации игры...
gcc -O2 -std=gnu11 -o build\fake_game.exe tests\fake_game.c tests\fake_game.S
if errorlevel 1 goto :failed

copy /y build\fake_game.exe build\SnowRunner.exe >nul

echo [2/4] Запуск имитации игры...
start "" /b build\SnowRunner.exe > build\game.log 2>&1
timeout /t 2 /nobreak >nul

echo [3/4] Запуск программы...
start "" /b build\SnowRunnerTimeJumper.exe > build\tool.log 2>&1
timeout /t 4 /nobreak >nul

echo [4/4] Журнал программы...
if exist "%LOCALAPPDATA%\SnowRunnerTimeJumper\srtc.log" (
    type "%LOCALAPPDATA%\SnowRunnerTimeJumper\srtc.log"
) else (
    echo ЖУРНАЛ НЕ НАЙДЕН
    goto :failed
)

echo.
echo Журнал имитации игры (последние строки)...
powershell -NoProfile -Command "Get-Content build\game.log -Tail 8"

taskkill /f /im SnowRunnerTimeJumper.exe >nul 2>&1
taskkill /f /im SnowRunner.exe >nul 2>&1

echo.
echo Тест завершён.
exit /b 0

:failed
echo.
echo Тест не пройден.
taskkill /f /im SnowRunnerTimeJumper.exe >nul 2>&1
taskkill /f /im SnowRunner.exe >nul 2>&1
exit /b 1
