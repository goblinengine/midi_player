# MidiPlayer (Godot 4.5 GDExtension)

A Godot 4.5 GDExtension addon that plays `.mid` files using TinySoundFont (SoundFont2 `.sf2`) synthesis.

## Folder layout

- `addons/midi_player/midi_player.gdextension` – Godot extension manifest
- `src/` – C++ sources
- `thirdparty/TinySoundFont/` – expects `tsf.h` and `tml.h`

## Dependencies

- Godot Engine 4.5
- `godot-cpp` (4.5 branch/tag)
- TinySoundFont (`tsf.h`) + TinyMidiLoader (`tml.h`)
- Python 3.8+ and SCons 4.x
- A C++ toolchain (Visual Studio Build Tools on Windows)

## Quick start (Windows)

1) Clone with submodules:

```bash
git clone --recursive https://github.com/yourusername/MidiPlayer.git
# Or if already cloned:
git submodule update --init --recursive
```

2) Build:

- `scons platform=windows target=template_debug arch=x86_64`

The resulting DLL will be copied into:
- `addons/midi_player/bin/`

3) Use in Godot:

- Enable the extension by having `addons/midi_player/midi_player.gdextension` in your project.
- Add a `MidiPlayer` node and call `load_soundfont()` + `load_midi()` + `play()`.

## Notes

- This implementation loads `.sf2` and `.mid` via Godot `FileAccess` (works with `res://` paths).
- Output uses `AudioStreamGenerator` internally.
