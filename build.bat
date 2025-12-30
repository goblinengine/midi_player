@echo off
REM Simple build wrapper for Windows that sets up MSVC environment

echo MidiPlayer Build Script (Windows)
echo ===================================
echo.

REM Find Visual Studio
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"

if not exist "%VSWHERE%" (
    echo ERROR: vswhere.exe not found. Please install Visual Studio 2019 or later.
    exit /b 1
)

for /f "usebackq tokens=*" %%i in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do (
    set "VSPATH=%%i"
)

if not defined VSPATH (
    echo ERROR: Visual Studio with C++ tools not found!
    echo Please install Visual Studio with "Desktop development with C++" workload.
    exit /b 1
)

echo Found Visual Studio at: %VSPATH%
echo.

REM Setup MSVC environment
set "VCVARSALL=%VSPATH%\VC\Auxiliary\Build\vcvarsall.bat"

if not exist "%VCVARSALL%" (
    echo ERROR: vcvarsall.bat not found!
    exit /b 1
)

echo Setting up MSVC environment...
call "%VCVARSALL%" x64 >nul 2>&1

if errorlevel 1 (
    echo ERROR: Failed to setup MSVC environment
    exit /b 1
)

echo.
echo Building MidiPlayer GDExtension...
echo Platform: windows
echo Target: %1
echo Arch: x86_64
echo.

REM Build with scons
if "%1"=="" (
    scons platform=windows target=template_debug arch=x86_64
) else (
    scons platform=windows target=%1 arch=x86_64
)

if errorlevel 1 (
    echo.
    echo Build FAILED!
    exit /b 1
)

echo.
echo Build succeeded!
echo Output: addons\midi_player\bin\
