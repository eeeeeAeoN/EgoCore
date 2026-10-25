@echo off
setlocal
cd /d "%~dp0"

echo =======================================================
echo   Building stb_compiler.exe (GUI + CLI Tool)
echo =======================================================

call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
if errorlevel 1 exit /b 1

if not exist "lib" mkdir lib

if not exist "lib\StaticTerrainBank.lib" (
    echo Building StaticTerrainBank.lib first...
    call build_lib.bat
)

if not exist "lib\imgui.lib" (
    echo Building imgui.lib...
    cl.exe /nologo /c /O2 /std:c++20 /EHsc /MD /Itools\imgui ^
        tools\imgui\imgui.cpp ^
        tools\imgui\imgui_draw.cpp ^
        tools\imgui\imgui_tables.cpp ^
        tools\imgui\imgui_widgets.cpp ^
        tools\imgui\imgui_impl_win32.cpp ^
        tools\imgui\imgui_impl_dx11.cpp
    if errorlevel 1 exit /b 1
    lib.exe /nologo /OUT:lib\imgui.lib imgui*.obj
    del imgui*.obj 2>nul
)

cl.exe /nologo /O2 /std:c++20 /EHsc /MD /Iinclude /Itools\imgui tools\compiler_main.cpp tools\gui.cpp lib\StaticTerrainBank.lib lib\imgui.lib /Fe:stb_compiler.exe
if errorlevel 1 exit /b 1

del *.obj 2>nul
echo =======================================================
echo   stb_compiler.exe built successfully!
echo =======================================================
