@echo off
REM ============================================================================
REM  Build the Silent Storm engine -- Release (Windows x64).
REM  Needs CMake on PATH and Visual Studio 2022+.
REM ============================================================================
setlocal
where cmake >nul 2>nul
if errorlevel 1 (
    echo [ERROR] CMake was not found on PATH.
    echo         Install CMake, or run this from a "Developer Command Prompt for VS 2022".
    pause
    exit /b 1
)
if not defined SDL3_DIR (
    echo [ERROR] Set SDL3_DIR to the cmake directory of SDL3 3.4.16.
    pause
    exit /b 1
)
cmake -S "%~dp0." -B "%~dp0build" -A x64 "-DSDL3_DIR=%SDL3_DIR%" || (pause & exit /b 1)
cmake --build "%~dp0build" --config Release || (pause & exit /b 1)
echo.
echo [OK] Build finished -- see  build\Release\Game.exe
pause
