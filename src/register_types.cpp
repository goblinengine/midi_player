#include "register_types.h"

#include <godot_cpp/godot.hpp>

#include "midi_player.h"

namespace godot {

void initialize_midi_player_module(ModuleInitializationLevel p_level) {
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}
	ClassDB::register_class<MidiPlayer>();
}

void uninitialize_midi_player_module(ModuleInitializationLevel p_level) {
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}
}

} // namespace godot

extern "C" {

GDExtensionBool GDE_EXPORT midi_player_library_init(const GDExtensionInterface *p_interface,
		GDExtensionClassLibraryPtr p_library,
		GDExtensionInitialization *p_initialization) {
	godot::GDExtensionBinding::InitObject init_obj(p_interface, p_library, p_initialization);
	init_obj.register_initializer(godot::initialize_midi_player_module);
	init_obj.register_terminator(godot::uninitialize_midi_player_module);
	init_obj.set_minimum_library_initialization_level(MODULE_INITIALIZATION_LEVEL_SCENE);
	return init_obj.init();
}

}