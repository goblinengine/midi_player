#include "midi_player.h"

#include <algorithm>
#include <vector>

#include <godot_cpp/classes/audio_server.hpp>
#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/time.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/object.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

#include "../thirdparty/TinySoundFont/tsf.h"
#include "../thirdparty/TinySoundFont/tml.h"

namespace godot {

static constexpr int k_block_frames = 64;

MidiPlayer::MidiPlayer() {
	set_process(true);
}

MidiPlayer::~MidiPlayer() {
	stop();
	if (midi) {
		tml_free(midi);
		midi = nullptr;
	}
	if (sf) {
		tsf_close(sf);
		sf = nullptr;
	}
}

void MidiPlayer::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_soundfont_path", "path"), &MidiPlayer::set_soundfont_path);
	ClassDB::bind_method(D_METHOD("get_soundfont_path"), &MidiPlayer::get_soundfont_path);
	ClassDB::add_property("MidiPlayer", PropertyInfo(Variant::STRING, "soundfont_path"), "set_soundfont_path", "get_soundfont_path");

	ClassDB::bind_method(D_METHOD("set_midi_path", "path"), &MidiPlayer::set_midi_path);
	ClassDB::bind_method(D_METHOD("get_midi_path"), &MidiPlayer::get_midi_path);
	ClassDB::add_property("MidiPlayer", PropertyInfo(Variant::STRING, "midi_path"), "set_midi_path", "get_midi_path");

	ClassDB::bind_method(D_METHOD("set_loop", "loop"), &MidiPlayer::set_loop);
	ClassDB::bind_method(D_METHOD("get_loop"), &MidiPlayer::get_loop);
	ClassDB::add_property("MidiPlayer", PropertyInfo(Variant::BOOL, "loop"), "set_loop", "get_loop");

	ClassDB::bind_method(D_METHOD("set_volume", "volume"), &MidiPlayer::set_volume);
	ClassDB::bind_method(D_METHOD("get_volume"), &MidiPlayer::get_volume);
	ClassDB::add_property("MidiPlayer", PropertyInfo(Variant::FLOAT, "volume", PROPERTY_HINT_RANGE, "0.0,2.0,0.01"), "set_volume", "get_volume");

	ClassDB::bind_method(D_METHOD("set_generator_buffer_length", "seconds"), &MidiPlayer::set_generator_buffer_length);
	ClassDB::bind_method(D_METHOD("get_generator_buffer_length"), &MidiPlayer::get_generator_buffer_length);
	ClassDB::add_property("MidiPlayer", PropertyInfo(Variant::FLOAT, "generator_buffer_length", PROPERTY_HINT_RANGE, "0.05,2.0,0.01"), "set_generator_buffer_length", "get_generator_buffer_length");

	ClassDB::bind_method(D_METHOD("load_soundfont", "path"), &MidiPlayer::load_soundfont);
	ClassDB::bind_method(D_METHOD("load_midi", "path"), &MidiPlayer::load_midi);

	ClassDB::bind_method(D_METHOD("play"), &MidiPlayer::play);
	ClassDB::bind_method(D_METHOD("stop"), &MidiPlayer::stop);
	ClassDB::bind_method(D_METHOD("pause"), &MidiPlayer::pause);
	ClassDB::bind_method(D_METHOD("resume"), &MidiPlayer::resume);
	ClassDB::bind_method(D_METHOD("is_playing"), &MidiPlayer::is_playing);

	ClassDB::bind_method(D_METHOD("get_length_seconds"), &MidiPlayer::get_length_seconds);
	ClassDB::bind_method(D_METHOD("get_playback_position_seconds"), &MidiPlayer::get_playback_position_seconds);
}

void MidiPlayer::_ready() {
	_ensure_audio_setup();
}

void MidiPlayer::_exit_tree() {
	stop();
}

void MidiPlayer::set_soundfont_path(const String &p_path) {
	soundfont_path = p_path;
}

String MidiPlayer::get_soundfont_path() const {
	return soundfont_path;
}

void MidiPlayer::set_midi_path(const String &p_path) {
	midi_path = p_path;
}

String MidiPlayer::get_midi_path() const {
	return midi_path;
}

void MidiPlayer::set_loop(bool p_loop) {
	loop = p_loop;
}

bool MidiPlayer::get_loop() const {
	return loop;
}

void MidiPlayer::set_volume(float p_volume) {
	volume = std::max(0.0f, p_volume);
	if (sf) {
		tsf_set_volume(sf, volume);
	}
}

float MidiPlayer::get_volume() const {
	return volume;
}

void MidiPlayer::set_generator_buffer_length(float p_seconds) {
	generator_buffer_length = std::max(0.05f, p_seconds);
	if (generator.is_valid()) {
		generator->set_buffer_length(generator_buffer_length);
	}
}

float MidiPlayer::get_generator_buffer_length() const {
	return generator_buffer_length;
}

PackedByteArray MidiPlayer::_read_all_bytes(const String &p_path) {
	PackedByteArray out;
	if (p_path.is_empty()) {
		return out;
	}

	Ref<FileAccess> f = FileAccess::open(p_path, FileAccess::READ);
	if (f.is_null()) {
		UtilityFunctions::push_error(String("MidiPlayer: Failed to open file: ") + p_path);
		return out;
	}

	int64_t len = f->get_length();
	if (len <= 0) {
		return out;
	}

	out.resize(len);
	PackedByteArray::Write w = out.write();
	f->get_buffer(w.ptr(), len);
	return out;
}

bool MidiPlayer::_load_soundfont_bytes(const PackedByteArray &p_bytes) {
	if (p_bytes.is_empty()) {
		UtilityFunctions::push_error("MidiPlayer: SoundFont bytes are empty.");
		return false;
	}

	if (sf) {
		tsf_close(sf);
		sf = nullptr;
	}

	PackedByteArray::Read r = p_bytes.read();
	sf = tsf_load_memory(r.ptr(), (int)p_bytes.size());
	if (!sf) {
		UtilityFunctions::push_error("MidiPlayer: tsf_load_memory() failed.");
		return false;
	}

	sample_rate = (int)AudioServer::get_singleton()->get_mix_rate();
	if (sample_rate <= 0) {
		sample_rate = 44100;
	}

	tsf_set_output(sf, TSF_STEREO_INTERLEAVED, sample_rate, 0.0f);
	tsf_set_max_voices(sf, 256);
	tsf_set_volume(sf, volume);

	// Initialize channels so channel allocation won't happen during playback.
	for (int ch = 0; ch < 16; ch++) {
		// Default program 0, drums on channel 9.
		tsf_channel_set_presetnumber(sf, ch, 0, ch == 9);
		// Set center pan + full volume in TSF's MIDI controller space.
		tsf_channel_midi_control(sf, ch, (int)TML_PAN_MSB, 64);
		tsf_channel_midi_control(sf, ch, (int)TML_VOLUME_MSB, 127);
	}

	return true;
}

bool MidiPlayer::_load_midi_bytes(const PackedByteArray &p_bytes) {
	if (p_bytes.is_empty()) {
		UtilityFunctions::push_error("MidiPlayer: MIDI bytes are empty.");
		return false;
	}

	if (midi) {
		tml_free(midi);
		midi = nullptr;
	}

	PackedByteArray::Read r = p_bytes.read();
	midi = tml_load_memory(r.ptr(), (int)p_bytes.size());
	if (!midi) {
		UtilityFunctions::push_error("MidiPlayer: tml_load_memory() failed.");
		return false;
	}

	unsigned int first_note_ms = 0;
	unsigned int length_ms = 0;
	tml_get_info(midi, nullptr, nullptr, nullptr, &first_note_ms, &length_ms);
	midi_length_ms = (uint32_t)length_ms;

	return true;
}

bool MidiPlayer::load_soundfont(const String &p_path) {
	soundfont_path = p_path;
	PackedByteArray bytes = _read_all_bytes(p_path);
	return _load_soundfont_bytes(bytes);
}

bool MidiPlayer::load_midi(const String &p_path) {
	midi_path = p_path;
	PackedByteArray bytes = _read_all_bytes(p_path);
	return _load_midi_bytes(bytes);
}

void MidiPlayer::_ensure_audio_setup() {
	if (!player) {
		player = memnew(AudioStreamPlayer);
		player->set_name("_MidiPlayerAudio");
		add_child(player);
	}

	sample_rate = (int)AudioServer::get_singleton()->get_mix_rate();
	if (sample_rate <= 0) {
		sample_rate = 44100;
	}

	if (!generator.is_valid()) {
		generator.instantiate();
		generator->set_mix_rate(sample_rate);
		generator->set_buffer_length(generator_buffer_length);
		player->set_stream(generator);
	}

	if (!player->is_playing()) {
		player->play();
	}

	playback_base = player->get_stream_playback();
	playback = Object::cast_to<AudioStreamGeneratorPlayback>(playback_base.ptr());
	if (!playback) {
		// If this ever happens, we can't output audio.
		UtilityFunctions::push_warning("MidiPlayer: AudioStreamGeneratorPlayback not available yet.");
	}
}

void MidiPlayer::_clear_audio_buffer() {
	if (!playback) {
		return;
	}
	// There is no explicit clear API; pushing nothing lets it drain.
	// We force a stop/play cycle to effectively reset the buffer.
	if (player) {
		player->stop();
		player->play();
		playback_base = player->get_stream_playback();
		playback = Object::cast_to<AudioStreamGeneratorPlayback>(playback_base.ptr());
	}
}

void MidiPlayer::_reset_synth() {
	if (!sf) {
		return;
	}
	tsf_reset(sf);
	// Re-apply volume since reset may clear channels.
	tsf_set_output(sf, TSF_STEREO_INTERLEAVED, sample_rate, 0.0f);
	tsf_set_max_voices(sf, 256);
	tsf_set_volume(sf, volume);
	for (int ch = 0; ch < 16; ch++) {
		tsf_channel_set_presetnumber(sf, ch, 0, ch == 9);
		tsf_channel_midi_control(sf, ch, (int)TML_PAN_MSB, 64);
		tsf_channel_midi_control(sf, ch, (int)TML_VOLUME_MSB, 127);
	}
}

void MidiPlayer::play() {
	_ensure_audio_setup();

	if (!sf) {
		if (!soundfont_path.is_empty()) {
			load_soundfont(soundfont_path);
		}
	}
	if (!midi) {
		if (!midi_path.is_empty()) {
			load_midi(midi_path);
		}
	}

	if (!sf || !midi) {
		UtilityFunctions::push_error("MidiPlayer: Cannot play (missing soundfont or midi). Call load_soundfont() and load_midi() first.");
		return;
	}

	_reset_synth();
	_clear_audio_buffer();

	event_cursor = midi;
	synth_time_sec = 0.0;
	playing = true;
	paused = false;

	if (player && !player->is_playing()) {
		player->play();
	}
}

void MidiPlayer::stop() {
	playing = false;
	paused = false;
	synth_time_sec = 0.0;
	event_cursor = midi;

	if (sf) {
		tsf_note_off_all(sf);
		tsf_reset(sf);
	}
	if (player) {
		player->stop();
	}
	playback_base.unref();
	playback = nullptr;
}

void MidiPlayer::pause() {
	if (!playing) {
		return;
	}
	paused = true;
	if (player) {
		player->stop();
	}
}

void MidiPlayer::resume() {
	if (!playing) {
		return;
	}
	paused = false;
	_ensure_audio_setup();
	if (player && !player->is_playing()) {
		player->play();
	}
}

bool MidiPlayer::is_playing() const {
	return playing && !paused;
}

float MidiPlayer::get_length_seconds() const {
	return (float)midi_length_ms / 1000.0f;
}

float MidiPlayer::get_playback_position_seconds() const {
	return (float)synth_time_sec;
}

void MidiPlayer::_apply_event(const tml_message *p_msg) {
	if (!sf || !p_msg) {
		return;
	}

	switch (p_msg->type) {
		case TML_NOTE_ON: {
			const float vel = (float)(uint8_t)p_msg->velocity / 127.0f;
			tsf_channel_note_on(sf, p_msg->channel, p_msg->key, vel);
		} break;
		case TML_NOTE_OFF: {
			tsf_channel_note_off(sf, p_msg->channel, p_msg->key);
		} break;
		case TML_CONTROL_CHANGE: {
			tsf_channel_midi_control(sf, p_msg->channel, (int)(uint8_t)p_msg->control, (int)(uint8_t)p_msg->control_value);
		} break;
		case TML_PROGRAM_CHANGE: {
			tsf_channel_set_presetnumber(sf, p_msg->channel, (int)(uint8_t)p_msg->program, p_msg->channel == 9);
		} break;
		case TML_PITCH_BEND: {
			tsf_channel_set_pitchwheel(sf, p_msg->channel, (int)p_msg->pitch_bend);
		} break;
		case TML_CHANNEL_PRESSURE:
		case TML_KEY_PRESSURE:
			// Not directly supported by TSF channel API.
			break;
		default:
			// Includes tempo meta messages and EOT. Times are already baked into Msg->time.
			break;
	}
}

void MidiPlayer::_process_events_until_ms(uint32_t p_time_ms) {
	while (event_cursor && event_cursor->time <= p_time_ms) {
		_apply_event(event_cursor);
		event_cursor = event_cursor->next;

		if (!event_cursor) {
			break;
		}
	}
}

void MidiPlayer::_pump_audio() {
	if (!sf || !playback) {
		return;
	}

	int frames_available = playback->get_frames_available();
	if (frames_available <= 0) {
		return;
	}

	std::vector<float> interleaved;
	interleaved.resize((size_t)k_block_frames * 2);

	while (frames_available > 0) {
		const int frames = std::min(frames_available, k_block_frames);
		const double block_end_sec = synth_time_sec + (double)frames / (double)sample_rate;
		const uint32_t block_end_ms = (uint32_t)(block_end_sec * 1000.0);

		_process_events_until_ms(block_end_ms);

		if ((int)interleaved.size() < frames * 2) {
			interleaved.resize((size_t)frames * 2);
		}

		tsf_render_float(sf, interleaved.data(), frames, 0);

		PackedVector2Array buf;
		buf.resize(frames);
		for (int i = 0; i < frames; i++) {
			const float l = interleaved[i * 2 + 0];
			const float r = interleaved[i * 2 + 1];
			buf.set(i, Vector2(l, r));
		}

		playback->push_buffer(buf);

		synth_time_sec = block_end_sec;
		frames_available -= frames;

		// If we're past the MIDI length and there are no active voices, stop.
		if (!event_cursor) {
			if (tsf_active_voice_count(sf) == 0) {
				if (loop) {
					play();
				}
				break;
			}
		}
	}
}

void MidiPlayer::_process(double p_delta) {
	(void)p_delta;
	if (!playing || paused) {
		return;
	}
	_ensure_audio_setup();
	_pump_audio();

	// Auto-stop when finished (non-loop).
	if (!loop && !event_cursor && sf && tsf_active_voice_count(sf) == 0) {
		stop();
	}
}

} // namespace godot
