#pragma once
#include "sparse_voxel_octree.h"
#include "voxel_node.h"
#include "voxel_utility.h"
#include <godot_cpp/classes/fast_noise_lite.hpp>
#include <godot_cpp/classes/static_body3d.hpp>


namespace godot {
class VoxelTerrain : public StaticBody3D, public VoxelNode {
  GDCLASS(VoxelTerrain, StaticBody3D)

public:
    // Minimum size will be 8 (TODO: Should enforce this)
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
  void initiate_fracture_propagation(Vector3 voxel_position, float initial_force);

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

  void _propagate_fracture(Vector3i voxel_position, float stress_intensity);
  float _get_voxel_toughness(Vector3i voxel_position);
  // TODO: Don't know what I need for this yet
  float _calculate_stress_intensity() {};

  // Generates new voxel grid rigid bodies by running a connected-component
  // algorithm to find groups of voxels disconnected from the main body of
  // terrain
  void _generate_bodies_from_disconnected_voxels();
  // Uses a Connected Component Labelling (CCL)
  // algorithm to find and remove voxels, allocating them into different grids
  // based on what group of voxels they are connected to.
  // Returns the extracted grids.
 std::vector<VoxelUtility::VoxelGrid> _extract_disconnected_voxel_groups();
 // Flood fill CCL implementation for extracting voxel grids
 std::vector<VoxelUtility::VoxelGrid> _flood_filled_connected_component_voxel_extraction();
 void _flood_fill(const int x, const int y, const int z, std::vector<bool>& visited_voxels,
     VoxelUtility::VoxelGrid& voxel_grid, std::vector<VoxelUtility::ExtractedVoxel>& extracted_voxels,
     Vector3i& grid_min_corner, Vector3i& grid_max_corner, std::vector<std::tuple<int,int,int>>& voxels_to_scan);
};
} // namespace godot
