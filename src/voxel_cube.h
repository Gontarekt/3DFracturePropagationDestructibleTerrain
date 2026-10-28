/*
 * Meshinstance3D must be added as a child with an 'ArrayMesh' type mesh attached.
 *
 * NOTE: Godot has a clockwise rendering order
 *
 * TODO:
 * Make a function for mesh generation
 * - Naive Voxel Mesh Generation
 *
 */

#pragma once

#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/core/math.hpp>
#include <godot_cpp/classes/mesh_instance3d.hpp>

namespace godot {

class VoxelCube : public Node3D {
	GDCLASS(VoxelCube, Node3D)

private:
	Vector3i size;
	int num_voxels; // Computed from size
	std::unique_ptr<uint8_t[]> voxels;
	MeshInstance3D* mesh_node;

protected:
	static void _bind_methods();

public:
	VoxelCube();
	~VoxelCube();

	void _ready() override;
	void _process(double delta) override;

	void set_size(const Vector3i new_size);
	Vector3i get_size() const { return size; }

	// Example Functions for helping understanding of godots rendering + procedural mesh generation
	void generate_triangle_strip_face();
	void generate_triangle_list_face();
};

} // namespace godot
