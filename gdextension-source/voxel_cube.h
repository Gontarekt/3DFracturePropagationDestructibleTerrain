#pragma once
#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/classes/static_body3d.hpp>
#include <godot_cpp/core/math.hpp>
#include <godot_cpp/classes/mesh_instance3d.hpp>
#include <godot_cpp/classes/collision_shape3d.hpp>
#include <vector>
#include "voxel_utility.h"

enum MeshGenerationType {
    triangle_strip_quad,
    triangle_list_quad,
    culling_voxel_mesh
};
VARIANT_ENUM_CAST(MeshGenerationType);

namespace godot {

class VoxelCube : public StaticBody3D {
	GDCLASS(VoxelCube, StaticBody3D)

private:
   	MeshInstance3D* _mesh_node;
	CollisionShape3D* _collider_node;

	std::unique_ptr<VoxelUtility::VoxelVector> _voxels;
	Vector3i _size;
	int _num_voxels; // Computed from size

	// Represent voxels with a simple integer value, 0 means no voxel, index calculated via helper functions to convert 3D to 1D array.
	int _num_tests_to_run;
	// Must set to true if you want the voxel mesh to regenerate
	bool _mesh_needs_generated;
	MeshGenerationType _mesh_generation_type;
	std::vector<double> _timings;

protected:
	static void _bind_methods();

public:
	VoxelCube();
	~VoxelCube();

	void _ready() override;
	void _process(double delta) override;

	void remove_voxel(Vector3 voxel_position);

	// GETTERS & SETTERS

	void set_num_tests_to_run(const int new_num_tests_to_run);
	int get_num_tests_to_run() const { return _num_tests_to_run; }
	void set_mesh_generation_type(const MeshGenerationType new_mesh_generation_type) { _mesh_generation_type = new_mesh_generation_type;};
	MeshGenerationType get_mesh_generation_type() const { return _mesh_generation_type; }
	void set_size(const Vector3i new_size);
	Vector3i get_size() const { return _size; }

private:
    // Mesh Generation
    // Example Functions for helping understanding of godots rendering + procedural mesh generation

	void _generate_mesh();
	void _generate_triangle_strip_quad(); // Test function
	void _generate_triangle_list_quad(); // Test function
	void _generate_voxel_mesh();


	// Other Private Functions


	void _generate_collider();
};

} // namespace godot
