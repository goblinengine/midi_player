# MidiPlayer GDExtension - Updated build instructions

All dependencies are now git submodules.

## Setup

```bash
# Clone with submodules
git clone --recursive <your-repo-url>

# Or if already cloned:
git submodule update --init --recursive
```

## Building

### Windows

**Option 1: Using MinGW-w64 (Recommended - Easier Setup)**

1. Install MinGW-w64:
   - Via MSYS2: `pacman -S mingw-w64-x86_64-gcc`
   - Or download from: https://www.mingw-w64.org/
   - Make sure `g++.exe` is in your PATH

2. Install Python 3.8+ and SCons:
   ```bash
   pip install scons
   ```

3. Build:
   ```bash
   scons platform=windows target=template_debug arch=x86_64
   ```

**Option 2: Using MSVC**

The godot-cpp build system should auto-detect MSVC if MinGW is not available, but this seems unreliable. If you have Visual Studio Build Tools installed, MinGW is still recommended for simpler setup.

### Linux

```bash
sudo apt-get install build-essential scons pkg-config
scons platform=linux target=template_debug arch=x86_64
```

### macOS

```bash
brew install scons
scons platform=macos target=template_debug arch=universal
```

## Build Targets

- `template_debug` - Debug builds (development)
- `template_release` - Release builds (production)

## Output

Built libraries will be in: `addons/midi_player/bin/`

## CI/CD

GitHub Actions workflows are configured in `.github/workflows/build.yml` to automatically build for all platforms.
