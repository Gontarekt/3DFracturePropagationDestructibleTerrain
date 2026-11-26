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
    // Based these toughness values roughly on the article: https://dl.acm.org/doi/epdf/10.1145/2766896
    // Toughness: 1.0E+5 +50%
    //static constexpr float MINIMUM_TOUGHNESS = 100000.f;
    //static constexpr float MAXIMUM_TOUGHNESS = 150000.f;

    Ref<FastNoiseLite> terrain_noise_generator;
    Ref<FastNoiseLite> toughness_noise_generator;
    //int terrain_noise_seed;
    //int toughness_noise_seed;

    bool _check_disconnected_components = false;
    bool _draw_toughness_debug_mesh = true;
    float _noise_frequency = 0.02f;
    float _toughness_noise_frequency = 0.1f;
    float _noise_voxel_spawn_threshold = 0.3f;
    FastNoiseLite::NoiseType _noise_type = FastNoiseLite::TYPE_SIMPLEX;
    FastNoiseLite::NoiseType _toughness_noise_type = FastNoiseLite::TYPE_SIMPLEX;
    float max_toughness = 150000.f;
    float min_toughness = 100000.f;


protected:
  static void _bind_methods();

public:
  VoxelTerrain();
  ~VoxelTerrain();

  void _ready() override;
  void _process(double delta) override;

  void remove_voxel(Vector3i voxel_position) override;
  VoxelUtility::Voxel get_voxel(Vector3i voxel_position) override;
  void initiate_fracture_propagation(Vector3 voxel_position, float initial_force);

  void create_solid_cube_SVO();
  void create_noise_terrain_SVO();

  // Getters & Setters

  PROPERTY_GETTER_AND_SETTERS(bool, check_disconnected_components)
  PROPERTY_GETTER_AND_SETTERS(bool, draw_toughness_debug_mesh)
  PROPERTY_GETTER_AND_SETTERS(float, noise_voxel_spawn_threshold)

  void set_noise_frequency(const float new_noise_frequency);
  float get_noise_frequency() const { return _noise_frequency; }

  void set_toughness_noise_frequency(const float new_toughness_noise_frequency);
  float get_toughness_noise_frequency() const { return _toughness_noise_frequency; }

  void set_noise_type(const FastNoiseLite::NoiseType new_noise_type);
  FastNoiseLite::NoiseType get_noise_type() const { return _noise_type; }

  void set_toughness_noise_type(const FastNoiseLite::NoiseType new_toughness_noise_type);
  FastNoiseLite::NoiseType get_toughness_noise_type() const { return _toughness_noise_type; }


private:
  // For debugging the morton codes
  void _morton_code_test();

  void _process_geometry_updates() override;
  void _generate_mesh() override;
  void _generate_culling_voxel_mesh();
  void _generate_toughness_debug_mesh();
  void _generate_collider() override;

  void _propagate_fracture(std::vector<VoxelUtility::FractureFront>& fracture_fronts, Vector3i voxel_position, float stress_intensity);
  float _get_voxel_toughness(Vector3i voxel_position);
  // TODO: Don't know what I need for this yet
  float _calculate_stress_intensity() { return -1.f; }; // TEMP RETURN

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
