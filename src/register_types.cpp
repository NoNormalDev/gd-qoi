#include <godot_cpp/classes/image_format_loader_extension.hpp>
#include <godot_cpp/core/class_db.hpp>

#include "register_types.h"
#include "qoi_module.h"

using namespace godot;

static Ref<ImageFormatLoaderQOI> image_loader;

void initialize_qoi_module(ModuleInitializationLevel p_level) {
    if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) return;
    
    ClassDB::register_class<ImageQOI>();
    ClassDB::register_class<ImageFormatLoaderQOI>();
    image_loader.instantiate();
    image_loader->add_format_loader();

}

void uninitialize_qoi_module(ModuleInitializationLevel p_level) {
    if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) return;
    
    if (image_loader.is_valid()) {
        image_loader->remove_format_loader();
        image_loader.unref();
    }
}

extern "C" {
GDExtensionBool GDE_EXPORT qoi_library_init(GDExtensionInterfaceGetProcAddress p_get_proc_address, 
                                           GDExtensionClassLibraryPtr p_library, 
                                           GDExtensionInitialization *r_initialization) {
    godot::GDExtensionBinding::InitObject init_obj(p_get_proc_address, p_library, r_initialization);
    init_obj.register_initializer(initialize_qoi_module);
    init_obj.register_terminator(uninitialize_qoi_module);
    init_obj.set_minimum_library_initialization_level(MODULE_INITIALIZATION_LEVEL_SCENE);
    return init_obj.init();
}
}