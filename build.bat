@echo off
setlocal
cd /d "%~dp0"

echo ==================================================
echo  SnowRunner Time Jumper - сборка
echo ==================================================
echo.

rem ---- 1. MinGW-w64 (gcc + windres) ----
where mingw32-make >nul 2>nul
if %errorlevel%==0 goto :mingw
where make >nul 2>nul
if %errorlevel%==0 goto :mingw

rem ---- 2. MSVC ----
where cl >nul 2>nul
if %errorlevel%==0 goto :msvc

rem ---- 3. CMake ----
where cmake >nul 2>nul
if %errorlevel%==0 goto :cmake

echo Не найден ни один компилятор.
echo.
echo Установите любое из:
echo   - MSYS2 + MinGW-w64 (gcc, windres, mingw32-make)
echo   - Visual Studio 2022 с компонентом "Разработка классических приложений на C++"
echo   - CMake (cmake + gcc/clang)
echo.
pause
exit /b 1

:mingw
echo [Сборка] MinGW-w64 (make)
where mingw32-make >nul 2>nul
if %errorlevel%==0 (mingw32-make clean ^& mingw32-make) else (make clean ^& make)
if errorlevel 1 goto :failed
goto :done

:msvc
echo [Сборка] Microsoft Visual C++
if not exist build mkdir build
cl /nologo /O2 /W3 /utf-8 /D_CRT_SECURE_NO_WARNINGS /Fe:build\SnowRunnerTimeJumper.exe ^
   src\*.c /link /subsystem:windows gdi32.lib gdiplus.lib user32.lib shell32.lib ^
   /MANIFEST:EMBED /MANIFESTINPUT:src\app.manifest
if errorlevel 1 goto :failed
goto :done

:cmake
echo [Сборка] CMake
if not exist build-cmake mkdir build-cmake
cmake -S . -B build-cmake -DCMAKE_BUILD_TYPE=Release
if errorlevel 1 goto :failed
cmake --build build-cmake --config Release
if errorlevel 1 goto :failed
goto :done

:failed
echo.
echo Сборка завершилась с ошибкой.
pause
exit /b 1

:done
echo.
echo Готово. Программа: build\SnowRunnerTimeJumper.exe
pause
exit /b 0
