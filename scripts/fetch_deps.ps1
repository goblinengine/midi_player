Param(
  [string]$GodotCppRef = "4.5",
  [string]$TinySoundFontRef = "master"
)

$ErrorActionPreference = "Stop"

$root = Split-Path -Parent $PSScriptRoot

function Ensure-Dir($path) {
  if (!(Test-Path $path)) { New-Item -ItemType Directory -Force -Path $path | Out-Null }
}

Write-Host "Fetching godot-cpp ($GodotCppRef)..."
Push-Location $root
try {
  if (!(Test-Path "$root\godot-cpp")) {
    git clone --depth 1 --branch $GodotCppRef https://github.com/godotengine/godot-cpp.git godot-cpp
  } else {
    Write-Host "godot-cpp already exists, skipping clone."
  }
} finally {
  Pop-Location
}

Write-Host "Fetching TinySoundFont headers ($TinySoundFontRef)..."
$tsfDir = Join-Path $root "thirdparty\TinySoundFont"
Ensure-Dir $tsfDir

$base = "https://raw.githubusercontent.com/schellingb/TinySoundFont/$TinySoundFontRef"
Invoke-WebRequest -Uri "$base/tsf.h" -OutFile (Join-Path $tsfDir "tsf.h")
Invoke-WebRequest -Uri "$base/tml.h" -OutFile (Join-Path $tsfDir "tml.h")

Write-Host "Done."