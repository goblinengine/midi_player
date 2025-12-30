#include "register_types.h"

#include <godot_cpp/godot.hpp>
#include <godot_cpp/core/defs.hpp>
#include <godot_cpp/core/class_db.hpp>

#include "midi_player.h"
#include "midi_resources.h"
#include "midi_importers.h"

namespace godot {

void initialize_midi_player_module(ModuleInitializationLevel p_level) {
	if (p_level == MODULE_INITIALIZATION_LEVEL_SCENE) {
		ClassDB::register_class<MidiFileResource>();
		ClassDB::register_class<SoundFontResource>();
		ClassDB::register_class<MidiPlayer>();
	}
	if (p_level == MODULE_INITIALIZATION_LEVEL_EDITOR) {
		ClassDB::register_class<MidiImporter>();
		ClassDB::register_class<SoundFontImporter>();
	}
}

void uninitialize_midi_player_module(ModuleInitializationLevel p_level) {
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}
}

} // namespace godot

// C linkage entry point - signature matches GDExtension requirements
extern "C" {
uint32_t GDE_EXPORT midi_player_library_init(GDExtensionInterfaceGetProcAddress p_get_proc_address,
		GDExtensionClassLibraryPtr p_library,
		GDExtensionInitialization *p_initialization) {
	godot::GDExtensionBinding::InitObject init_obj(p_get_proc_address, p_library, p_initialization);
	init_obj.register_initializer(godot::initialize_midi_player_module);
	init_obj.register_terminator(godot::uninitialize_midi_player_module);
	init_obj.set_minimum_library_initialization_level(godot::MODULE_INITIALIZATION_LEVEL_SCENE);
	return init_obj.init();
}
}