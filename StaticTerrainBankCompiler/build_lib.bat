@echo off
setlocal
cd /d "%~dp0"

echo =======================================================
echo   Building StaticTerrainBank Static Library (x64)
echo =======================================================

call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
if errorlevel 1 (
    echo Failed to initialize MSVC 64-bit environment.
    exit /b 1
)

if not exist "lib" mkdir lib

echo Compiling source files...
cl.exe /nologo /c /O2 /std:c++20 /EHsc /MD /W3 /Iinclude ^
    src\bbb_archive.cpp ^
    src\bank_resolver.cpp ^
    src\background_tree.cpp ^
    src\clutter.cpp ^
    src\dxt1_baker.cpp ^
    src\heightmap.cpp ^
    src\lev_reader.cpp ^
    src\local_detail_builder.cpp ^
    src\local_detail_manager.cpp ^
    src\lzo1x.cpp ^
    src\mesh_builder.cpp ^
    src\range_compressor.cpp ^
    src\sea_generator.cpp ^
    src\stb_compiler.cpp ^
    src\stripifier.cpp ^
    src\theme_manager.cpp

if errorlevel 1 (
    echo Compilation failed!
    del *.obj 2>nul
    exit /b 1
)

echo Creating static library lib\StaticTerrainBank.lib...
lib.exe /nologo /OUT:lib\StaticTerrainBank.lib *.obj
if errorlevel 1 (
    echo Archiving failed!
    del *.obj 2>nul
    exit /b 1
)

del *.obj 2>nul
echo =======================================================
echo   StaticTerrainBank.lib created successfully in lib\
echo =======================================================
exit /b 0
