@echo off
setlocal
cd /d "%~dp0"

echo =======================================================
echo   Building stb_compiler.exe (CLI Tool)
echo =======================================================

call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
if errorlevel 1 exit /b 1

if not exist "lib\StaticTerrainBank.lib" (
    echo Building StaticTerrainBank.lib first...
    call build_lib.bat
)

cl.exe /nologo /O2 /std:c++20 /EHsc /MD /Iinclude tools\compiler_main.cpp lib\StaticTerrainBank.lib /Fe:stb_compiler.exe
if errorlevel 1 exit /b 1

del *.obj 2>nul
echo =======================================================
echo   stb_compiler.exe built successfully!
echo =======================================================
