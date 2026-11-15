#include "voxel_node.h"
#include "godot_cpp/classes/array_mesh.hpp"
#include "godot_cpp/core/print_string.hpp"


VoxelNode::VoxelNode()
{

}


///////////////////
// Public Functions


// Nothing yet


////////////////////
// Getters & Setters


void VoxelNode::set_num_mesh_generation_tests_to_run(const int new_num_mesh_generation_tests_to_run)
{
    _num_mesh_generation_tests_to_run = new_num_mesh_generation_tests_to_run;
    _num_mesh_generation_tests_to_run = std::max(_num_mesh_generation_tests_to_run, 1);
}


////////////////////
// Protected Functions


void VoxelNode::_initialise_nodes(godot::Node* this_node)
{
    godot::MeshInstance3D* new_mesh_node = memnew(godot::MeshInstance3D);
    this_node->add_child(new_mesh_node);
    _mesh_node = new_mesh_node;
    godot::ArrayMesh* new_array_mesh = memnew(godot::ArrayMesh);
    _mesh_node->set_mesh(new_array_mesh);

    godot::CollisionShape3D* new_collider_node = memnew(godot::CollisionShape3D);
    this_node->add_child(new_collider_node);
    _collider_node = new_collider_node;
}


void VoxelNode::_process_geometry_updates()
{
    if (!_voxel_grid_initialised)
    {
        //CRASH_NOW_MSG("VoxelNode was not initialised! Did you forget to set the initialised bool to true or did you forget to create a grid for the node?");
        godot::print_error("VoxelNode was not initialised! Did you forget to set the initialised bool to true or did you forget to create a grid for the node?");
        return;
    }

    if (_mesh_needs_generated)
    {
        _generate_mesh();
        _mesh_needs_generated = false;
    }
    if (_collider_needs_generated)
    {
        _generate_collider();
        _collider_needs_generated = false;
    }
}
