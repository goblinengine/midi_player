#!/usr/bin/env python

import os

# This project expects godot-cpp to be available at ./godot-cpp
# (either as a git submodule, or cloned via scripts/fetch_deps.ps1)

if not os.path.isdir("godot-cpp"):
    print("ERROR: Missing ./godot-cpp. Run: powershell -File scripts/fetch_deps.ps1")
    Exit(1)

# Workaround for Windows MSVC: If building with MSVC and include paths are empty,
# manually set them from the environment (which should be set by vcvarsall.bat)
if os.name == 'nt':
    include_env = os.environ.get('INCLUDE', '')
    lib_env = os.environ.get('LIB', '')
    if include_env:
        os.environ['INCLUDE'] = include_env
    if lib_env:
        os.environ['LIB'] = lib_env

# Build godot-cpp and reuse its configured env (platform/target/arch/etc).

godot_env = SConscript("godot-cpp/SConstruct")

env = godot_env.Clone()

# Project sources
sources = [
    "src/midi_player.cpp",
    "src/register_types.cpp",
    "src/thirdparty_tsf_tml.cpp",
]

env.AppendUnique(CPPPATH=[
    "src",
    "thirdparty/TinySoundFont-upstream",
])

# Build output naming.
# godot-cpp exposes env['suffix'] like: .windows.template_debug.x86_64
suffix = env.get("suffix", "")

# Shared library naming differs by platform.
# - Windows: midi_player.windows.template_debug.x86_64.dll
# - Other:   libmidi_player.linux.template_debug.x86_64.so
lib_basename = "midi_player" + suffix

# Emit into the addon bin folder so Godot can load it.
out_dir = "addons/midi_player/bin"

# Ensure output dir exists.
if not os.path.isdir(out_dir):
    os.makedirs(out_dir)

# SCons' SharedLibrary will add platform-specific prefixes/suffixes.
# For Windows we want no 'lib' prefix, for others 'lib' is fine.
if env["platform"] == "windows":
    env["SHLIBPREFIX"] = ""

# Copy/rename the output to exactly what our .gdextension expects.
# We set the target filename explicitly.

target_path = os.path.join(out_dir, lib_basename + env["SHLIBSUFFIX"])

lib = env.SharedLibrary(
    target=env.File(target_path),
    source=sources,
)

# Default build target.
Default(lib)
