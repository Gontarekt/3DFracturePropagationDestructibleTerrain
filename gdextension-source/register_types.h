/*
 * Remember that new classes / node types need to be added to the 'initialize_example_module()' function.
 *
 */

#pragma once

#include <godot_cpp/core/class_db.hpp>

using namespace godot;

void initialize_example_module(ModuleInitializationLevel p_level);
void uninitialize_example_module(ModuleInitializationLevel p_level);
