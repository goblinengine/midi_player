#pragma once

#include <cstdint>

#include <godot_cpp/classes/audio_stream_generator.hpp>
#include <godot_cpp/classes/audio_stream_generator_playback.hpp>
#include <godot_cpp/classes/audio_stream_player.hpp>
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/templates/vector.hpp>

// TinySoundFont / TinyMidiLoader forward declarations.
struct tsf;
struct tml_message;

namespace godot {

class MidiPlayer : public Node {
	GDCLASS(MidiPlayer, Node)

public:
	MidiPlayer();
	~MidiPlayer();

	void set_soundfont_path(const String &p_path);
	String get_soundfont_path() const;

	void set_midi_path(const String &p_path);
	String get_midi_path() const;

	void set_loop(bool p_loop);
	bool get_loop() const;

	void set_looping(bool p_looping);
	bool is_looping() const;

	void set_midi_speed(float p_speed);
	float get_midi_speed() const;

	void set_volume(float p_volume);
	float get_volume() const;

	void set_generator_buffer_length(float p_seconds);
	float get_generator_buffer_length() const;

	bool load_soundfont(const String &p_path);
	bool load_midi(const String &p_path);

	void play();
	void stop();
	void pause();
	void resume();
	bool is_playing() const;

	void note_on(int p_preset_index, int p_key, float p_velocity);
	void note_off(int p_preset_index, int p_key);
	void note_off_all();

	float get_length_seconds() const;
	float get_playback_position_seconds() const;

	// Virtual methods (public for godot-cpp binding)
	void _ready() override;
	void _exit_tree() override;
	void _process(double p_delta) override;

protected:
	static void _bind_methods();
	void _ensure_audio_setup();
	void _clear_audio_buffer();
	void _reset_synth();
	bool _load_soundfont_bytes(const PackedByteArray &p_bytes);
	bool _load_midi_bytes(const PackedByteArray &p_bytes);
	static PackedByteArray _read_all_bytes(const String &p_path);
	void _apply_event(const tml_message *p_msg);
	void _process_events_until_ms(uint32_t p_time_ms);
	void _pump_audio();

	String soundfont_path;
	String midi_path;

	bool loop = false;
	float volume = 1.0f; // linear gain
	float midi_speed = 1.0f; // playback speed multiplier
	float generator_buffer_length = 0.5f;

	// Godot audio output
	AudioStreamPlayer *player = nullptr;
	Ref<AudioStreamGenerator> generator;
	Ref<AudioStreamPlayback> playback_base;
	AudioStreamGeneratorPlayback *playback = nullptr; // borrowed from playback_base
	int sample_rate = 44100;

	// Synth/midi
	tsf *sf = nullptr;
	tml_message *midi = nullptr;
	tml_message *event_cursor = nullptr;

	uint32_t midi_length_ms = 0;
	bool playing = false;
	bool paused = false;

	// Amount of audio already generated since play() in seconds.
	double synth_time_sec = 0.0;
};

} // namespace godot
