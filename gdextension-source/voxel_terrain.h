#pragma once
#include "sparse_voxel_octree.h"
#include "voxel_node.h"
#include <godot_cpp/classes/fast_noise_lite.hpp>
#include <godot_cpp/classes/static_body3d.hpp>


namespace godot {
class VoxelTerrain : public StaticBody3D, public VoxelNode {
  GDCLASS(VoxelTerrain, StaticBody3D)

public:
    static constexpr int terrain_chunk_size = 64;
    SparseVoxelOctree sparse_voxel_octree;

private:
    bool _check_disconnected_components = false;
    float _noise_frequency;
    float _noise_voxel_spawn_threshold;
    FastNoiseLite::NoiseType _noise_type;


protected:
  static void _bind_methods();

public:
  VoxelTerrain();
  ~VoxelTerrain();

  void _ready() override;
  void _process(double delta) override;

  void remove_voxel(Vector3 voxel_position) override;

  void create_solid_cube_SVO();
  void create_noise_terrain_SVO();

  // Getters & Setters

  void set_check_disconnected_components(const bool new_check_disconnected_components) { _check_disconnected_components = new_check_disconnected_components; };
  bool get_check_disconnected_components() const { return _check_disconnected_components; }
  void set_noise_frequency(const float new_noise_frequency) { _noise_frequency = new_noise_frequency; };
  float get_noise_frequency() const { return _noise_frequency; }
  void set_noise_voxel_spawn_threshold(const float new_noise_voxel_spawn_threshold) { _noise_voxel_spawn_threshold = new_noise_voxel_spawn_threshold; };
  float get_noise_voxel_spawn_threshold() const { return _noise_voxel_spawn_threshold; }
  void set_noise_type(const FastNoiseLite::NoiseType new_noise_type) { _noise_type = new_noise_type; };
  FastNoiseLite::NoiseType get_noise_type() const { return _noise_type; }

private:
  // For debugging the morton codes
  void _morton_code_test();

  void _process_geometry_updates() override;
  void _generate_mesh() override;
  void _generate_culling_voxel_mesh();
  void _generate_collider() override;
  // Generates new voxel grid rigid bodies by running a connected-component
  // algorithm to find groups of voxels disconnected from the main body of
  // terrain
  void _generate_bodies_from_disconnected_voxels();

  // Returns a vector of voxel grids that are disconnected from the main terrain
  // body and removes them from the terrain chunk. Uses a connected-component
  // algorithm. Also returns the position of the very first voxel (Lowest
  // position in all axis) so the original position is saved. (Any voxels
  // connected to unedited voxel chunks are part of the main body)
 std::vector<VoxelUtility::VoxelGrid> _extract_disconnected_voxel_groups();
};
} // namespace godot
