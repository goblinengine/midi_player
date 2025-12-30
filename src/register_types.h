#pragma once

#include <godot_cpp/core/class_db.hpp>

namespace godot {

void initialize_midi_player_module(godot::ModuleInitializationLevel p_level);
void uninitialize_midi_player_module(godot::ModuleInitializationLevel p_level);

} // namespace godot
