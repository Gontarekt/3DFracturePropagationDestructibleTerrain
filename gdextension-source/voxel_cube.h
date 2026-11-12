#pragma once
#include <godot_cpp/classes/static_body3d.hpp>
#include <godot_cpp/core/math.hpp>
#include "voxel_node.h"

enum MeshGenerationType {
    triangle_strip_quad,
    triangle_list_quad,
    culling_voxel_mesh
};
VARIANT_ENUM_CAST(MeshGenerationType);

namespace godot {

class VoxelCube : public StaticBody3D, public VoxelNode {
	GDCLASS(VoxelCube, StaticBody3D)

private:
	std::unique_ptr<VoxelUtility::VoxelVector> _voxels;
	Vector3i _size;
	int _num_voxels; // Computed from size

	MeshGenerationType _mesh_generation_type;

protected:
	static void _bind_methods();

public:
	VoxelCube();
	~VoxelCube();

	// Godot Functions

	void _ready() override;
	void _process(double delta) override;

	// Public Functions

	void remove_voxel(Vector3 voxel_position) override;

	// Getters & Setters

	void set_mesh_generation_type(const MeshGenerationType new_mesh_generation_type) { _mesh_generation_type = new_mesh_generation_type;};
	MeshGenerationType get_mesh_generation_type() const { return _mesh_generation_type; }
	void set_size(const Vector3i new_size);
	Vector3i get_size() const { return _size; }

private:
    // Mesh Generation
    // Example Functions for helping understanding of godots rendering + procedural mesh generation

	void _generate_mesh() override;
	void _generate_triangle_strip_quad(); // Test function
	void _generate_triangle_list_quad(); // Test function
	void _generate_voxel_mesh();

	// Private Functions

	void _generate_collider() override;
};
} // namespace godot
