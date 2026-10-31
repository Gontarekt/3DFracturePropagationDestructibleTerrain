/*
 * Meshinstance3D must be added as a child with an 'ArrayMesh' type mesh attached.
 *
 * NOTE: Godot has a clockwise rendering order
 *
 * TODO:
 * Make a function for mesh generation
 * - Naive Voxel Mesh Generation
 * Possibly have enum for type of mesh generation (Simple quad test, naive voxel mesh, etc)
 * Fix naming convention (prefix with _ for private variables)
 */

#pragma once

#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/classes/static_body3d.hpp>
#include <godot_cpp/core/math.hpp>
#include <godot_cpp/classes/mesh_instance3d.hpp>
#include <vector>

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
	Vector3i size;
	int num_voxels; // Computed from size
	// Represent voxels with a simple integer value, 0 means no voxel, index calculated via helper functions to convert 3D to 1D array.
	std::unique_ptr<std::vector<uint8_t>> voxels;
	MeshInstance3D* mesh_node;
	int num_tests_to_run;
	bool mesh_needs_generated;
	std::vector<double> timings;
	MeshGenerationType mesh_generation_type;

protected:
	static void _bind_methods();

public:
	VoxelCube();
	~VoxelCube();

	void _ready() override;
	void _process(double delta) override;

	void set_num_tests_to_run(const int new_num_tests_to_run);
	int get_num_tests_to_run() const { return num_tests_to_run; }

	void set_mesh_generation_type(const MeshGenerationType new_mesh_generation_type) { mesh_generation_type = new_mesh_generation_type;};
	MeshGenerationType get_mesh_generation_type() const { return mesh_generation_type; }

	void set_size(const Vector3i new_size);
	Vector3i get_size() const { return size; }

	// Example Functions for helping understanding of godots rendering + procedural mesh generation
	void generate_mesh();
	void generate_triangle_strip_quad(); // Test function
	void generate_triangle_list_quad(); // Test function
	void generate_voxel_mesh();
	void generate_collider();
};

} // namespace godot
