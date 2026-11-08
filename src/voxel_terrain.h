#pragma once

#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/classes/static_body3d.hpp>
#include <godot_cpp/classes/mesh_instance3d.hpp>
#include <godot_cpp/classes/collision_shape3d.hpp>
#include "sparse_voxel_octree.h"
#include <godot_cpp/classes/fast_noise_lite.hpp>

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
	float noise_frequency;
	FastNoiseLite::NoiseType noise_type;
	float noise_voxel_spawn_threshold;

protected:
	static void _bind_methods();

public:
	VoxelTerrain();
	~VoxelTerrain();

	void _ready() override;
	void _process(double delta) override;

	void remove_voxel(Vector3 voxel_position);

	void set_num_tests_to_run(const int new_num_tests_to_run);
	int get_num_tests_to_run() const { return num_tests_to_run; }
	void set_mesh_needs_generated(const bool new_mesh_needs_generated) {mesh_needs_generated = new_mesh_needs_generated;};
	bool get_mesh_needs_generated() const { return mesh_needs_generated; }
	void set_noise_frequency(const float new_noise_frequency) { noise_frequency = new_noise_frequency; };
	float get_noise_frequency() const { return noise_frequency; }
	void set_noise_type(const FastNoiseLite::NoiseType new_noise_type) { noise_type = new_noise_type; };
	FastNoiseLite::NoiseType get_noise_type() const { return noise_type; }
	void set_noise_voxel_spawn_threshold(const float new_noise_voxel_spawn_threshold) { noise_voxel_spawn_threshold = new_noise_voxel_spawn_threshold; };
	float get_noise_voxel_spawn_threshold() const { return noise_voxel_spawn_threshold; }

	void morton_code_test();
	void create_solid_cube_SVO();
	void create_noise_terrain_SVO();

	void generate_mesh();
	void generate_culling_voxel_mesh();
	void generate_collider();

	static constexpr int terrain_chunk_size = 64;
	SparseVoxelOctree sparse_voxel_octree;
};

} // namespace godot
