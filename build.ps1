# Build script for MidiPlayer GDExtension
# This script attempts to find and use the Visual Studio build tools

Param(
    [string]$Platform = "windows",
    [string]$Target = "template_debug",
    [string]$Arch = "x86_64",
    [int]$Jobs = 0
)

$ErrorActionPreference = "Stop"

Write-Host "MidiPlayer Build Script" -ForegroundColor Cyan
Write-Host "======================" -ForegroundColor Cyan
Write-Host ""

# Try to find Visual Studio
$vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"

if (Test-Path $vswhere) {
    Write-Host "Finding Visual Studio installation..." -ForegroundColor Yellow
    $vsPath = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
    
    if ($vsPath) {
        Write-Host "Found Visual Studio at: $vsPath" -ForegroundColor Green
        
        # Find vcvarsall.bat
        $vcvarsall = Join-Path $vsPath "VC\Auxiliary\Build\vcvarsall.bat"
        
        if (Test-Path $vcvarsall) {
            Write-Host "Setting up MSVC environment and building..." -ForegroundColor Yellow
            
            # Create a batch file that sets up MSVC and runs SCons
            $tempBat = [System.IO.Path]::GetTempFileName() + ".bat"
            
            $jobsArg = if ($Jobs -gt 0) { "-j$Jobs" } else { "" }
            
            @"
@echo off
call "$vcvarsall" x64
if errorlevel 1 exit /b 1
scons platform=$Platform target=$Target arch=$Arch $jobsArg
exit /b %errorlevel%
"@ | Out-File -FilePath $tempBat -Encoding ASCII
            
            # Run the batch file
            $result = cmd /c $tempBat
            $exitCode = $LASTEXITCODE
            
            # Clean up
            Remove-Item $tempBat -ErrorAction SilentlyContinue
            
            if ($exitCode -eq 0) {
                Write-Host ""
                Write-Host "Build succeeded!" -ForegroundColor Green
                Write-Host "Binary location: addons\midi_player\bin\" -ForegroundColor Cyan
            } else {
                Write-Host ""
                Write-Host "Build failed with exit code $exitCode" -ForegroundColor Red
            }
            exit $exitCode
        } else {
            Write-Host "Warning: vcvarsall.bat not found. Build may fail." -ForegroundColor Red
        }
    } else {
        Write-Host "Warning: Visual Studio with C++ tools not found!" -ForegroundColor Red
        Write-Host "Please install Visual Studio 2019 or later with C++ Desktop Development workload." -ForegroundColor Yellow
    }
} else {
    Write-Host "Warning: vswhere.exe not found. Cannot auto-detect Visual Studio." -ForegroundColor Red
    Write-Host "Attempting build anyway..." -ForegroundColor Yellow
}

Write-Host ""
Write-Host "Building with parameters:" -ForegroundColor Cyan
Write-Host "  Platform: $Platform" -ForegroundColor White
Write-Host "  Target:   $Target" -ForegroundColor White
Write-Host "  Arch:     $Arch" -ForegroundColor White
Write-Host ""
Write-Host "Attempting build without Visual Studio detection..." -ForegroundColor Yellow
Write-Host ""

# Build command
$sconsArgs = @("platform=$Platform", "target=$Target", "arch=$Arch")
if ($Jobs -gt 0) {
    $sconsArgs += "-j$Jobs"
}

& scons @sconsArgs

if ($LASTEXITCODE -eq 0) {
    Write-Host ""
    Write-Host "Build succeeded!" -ForegroundColor Green
    Write-Host "Binary location: addons\midi_player\bin\" -ForegroundColor Cyan
} else {
    Write-Host ""
    Write-Host "Build failed with exit code $LASTEXITCODE" -ForegroundColor Red
    Write-Host "Make sure you have Visual Studio with C++ tools installed." -ForegroundColor Yellow
    exit $LASTEXITCODE
}
