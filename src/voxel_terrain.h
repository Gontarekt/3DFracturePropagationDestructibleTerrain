#pragma once

#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/classes/static_body3d.hpp>
#include <godot_cpp/classes/mesh_instance3d.hpp>
#include <godot_cpp/classes/collision_shape3d.hpp>
#include "sparse_voxel_octree.h"

namespace godot {

class VoxelTerrain : public StaticBody3D {
	GDCLASS(VoxelTerrain, StaticBody3D)

private:
    MeshInstance3D* mesh_node;
	CollisionShape3D* collider_node;
	int num_tests_to_run;
	// Must set to true if you want the voxel mesh to regenerate
	bool mesh_needs_generated;
	std::vector<double> timings;

protected:
	static void _bind_methods();

public:
	VoxelTerrain();
	~VoxelTerrain();

	void _ready() override;
	void _process(double delta) override;

	void set_num_tests_to_run(const int new_num_tests_to_run);
	int get_num_tests_to_run() const { return num_tests_to_run; }

	void morton_code_test();
	void create_solid_cube_SVO();

	void generate_mesh();
	void generate_culling_voxel_mesh();
	void generate_collider();

	static constexpr int terrain_chunk_size = 64;
	SparseVoxelOctree sparse_voxel_octree;
};

} // namespace godot
