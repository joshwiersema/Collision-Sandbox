@echo off
rem Builds the sandbox and the tests with MSVC. Run from the project folder.
setlocal

rem Find the Visual Studio / Build Tools install and load the 64-bit compiler.
rem vswhere is run from its own folder so the "(x86)" in the path does not confuse cmd.
pushd "%ProgramFiles(x86)%\Microsoft Visual Studio\Installer"
for /f "usebackq tokens=*" %%i in (`.\vswhere.exe -latest -products * -property installationPath`) do set "VSPATH=%%i"
popd
if not defined VSPATH (
    echo Could not find Visual Studio. Install "Build Tools for Visual Studio" with the C++ workload.
    exit /b 1
)
call "%VSPATH%\VC\Auxiliary\Build\vcvars64.bat" >nul

if not exist build mkdir build
if not exist out mkdir out

set FLAGS=/nologo /std:c++17 /O2 /W4 /EHsc /Fobuild\

echo Building collision_sandbox.exe ...
cl %FLAGS% src\main.cpp src\Collision.cpp src\BruteForce.cpp src\SpatialGrid.cpp src\World.cpp src\PpmWriter.cpp /Febuild\collision_sandbox.exe
if errorlevel 1 exit /b 1

echo Building tests.exe ...
cl %FLAGS% tests\test_main.cpp src\Collision.cpp src\BruteForce.cpp src\SpatialGrid.cpp src\World.cpp src\PpmWriter.cpp /Febuild\tests.exe
if errorlevel 1 exit /b 1

echo.
echo Done. Run build\tests.exe then build\collision_sandbox.exe
