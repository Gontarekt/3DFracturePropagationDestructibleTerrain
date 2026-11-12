/*
 * Abstract Base Class
 * Implements the generic functions that all voxel nodes should have implementations for.
 * Derived node classes should use multi-inheritance to inherit the required godot node as well as VoxelNode.
 * Multi-inheritance is required as some VoxelNodes may want to inherit StaticBody nodes whereas others may
 * want to inherit RigidBody nodes.
 */

#pragma once
#include <godot_cpp/classes/mesh_instance3d.hpp>
#include <godot_cpp/classes/collision_shape3d.hpp>
#include "voxel_utility.h"

// This binds all properties and methods defined in VoxelNode to Godot.
// This macro should be called in _bind_methods() of derived classes.
// NOTE: methods added MUST be cast to the derived class otherwise the compiler
// will interpret them as VoxelNode functions which Godot can't use!
#define BIND_VOXEL_NODE_PROPERTIES(class_name) \
    godot::ClassDB::bind_method(godot::D_METHOD("get_mesh_needs_generated"), \
        static_cast<bool (class_name::*)() const>(&class_name::get_mesh_needs_generated)); \
	godot::ClassDB::bind_method(godot::D_METHOD("set_mesh_needs_generated", "new_mesh_needs_generated"), \
	    static_cast<void (class_name::*)(const bool)>(&class_name::set_mesh_needs_generated)); \
	ADD_PROPERTY(godot::PropertyInfo(godot::Variant::BOOL, "mesh_needs_generated"), "set_mesh_needs_generated", "get_mesh_needs_generated"); \
    \
    godot::ClassDB::bind_method(godot::D_METHOD("get_num_mesh_generation_tests_to_run"), \
	    static_cast<int (class_name::*)() const>(&class_name::get_num_mesh_generation_tests_to_run)); \
    godot::ClassDB::bind_method(godot::D_METHOD("set_num_mesh_generation_tests_to_run", "new_num_tests_to_run"), \
        static_cast<void (class_name::*)(const int)>(&class_name::set_num_mesh_generation_tests_to_run)); \
	ADD_PROPERTY(godot::PropertyInfo(godot::Variant::INT, "_num_mesh_generation_tests_to_run"), \
	    "set_num_mesh_generation_tests_to_run", "get_num_mesh_generation_tests_to_run"); \
	\
	godot::ClassDB::bind_method(godot::D_METHOD("remove_voxel", "voxel_position"), &class_name::remove_voxel); \


// May have to do this since Godot's macros for binding properties and methods doesn't recognize non-godot base classes properties/methods
//#define DECLARE_VOXEL_NODE_GODOT_ACCESSIBLE_METHODS(class_name) \


class VoxelNode {
    public:
        VoxelNode();
        ~VoxelNode() = default;

        virtual void remove_voxel(godot::Vector3 voxel_position) = 0;

        // Getters & Setters

        void set_mesh_needs_generated(const bool new_mesh_needs_generated) { _mesh_needs_generated = new_mesh_needs_generated; };
        bool get_mesh_needs_generated() const { return _mesh_needs_generated; }
        void set_num_mesh_generation_tests_to_run(const int new_num_mesh_generation_tests_to_run);
        int get_num_mesh_generation_tests_to_run() const { return _num_mesh_generation_tests_to_run; }

    protected:
        // MUST call this in _ready() so that the required nodes (e.g. MeshInstance3D) are created in time.
        // Pass the derived class node in. This is done as VoxelNode can't inherit godot nodes without the Diamond inheritance problem.
        virtual void _initialise_nodes(godot::Node* this_node);
        // Call this in _process(), checks for whether aspects of the voxel need regenerated and calls
        // the respective functions, e.g. mesh & collider.
        // Override if additional nodes or data needs updated but call the base implementation.
        virtual void _process_geometry_updates();
        virtual void _generate_mesh() = 0;
        virtual void _generate_collider() = 0;

    protected:
        godot::MeshInstance3D* _mesh_node = nullptr;
        godot::CollisionShape3D* _collider_node = nullptr;

        bool _mesh_needs_generated = true;
        bool _collider_needs_generated = true;
        int _num_mesh_generation_tests_to_run = 1;
        std::vector<double> _timings;
};
