# MidiPlayer addon

This folder contains the Godot addon manifest for the native GDExtension.

## Files

- `midi_player.gdextension` – loads the platform-specific compiled library
- `bin/` – build output folder (DLL/SO/DYLIB)

## Usage

1. Add a `MidiPlayer` node to your scene.
2. Call `load_soundfont("res://path/to/soundfont.sf2")`
3. Call `load_midi("res://path/to/song.mid")`
4. Call `play()`

Properties:
- `loop` (bool)
- `volume` (float, linear)
- `generator_buffer_length` (seconds)
