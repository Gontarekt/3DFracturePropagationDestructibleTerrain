#include "voxel_terrain.h"
#include "godot_cpp/classes/fast_noise_lite.hpp"
#include "godot_cpp/classes/global_constants.hpp"
#include <libmorton/morton.h>
#include <bitset>
#include "godot_cpp/variant/utility_functions.hpp"
#include "voxel_node.h"
#include "voxel_object.h"
#include "voxel_utility.h"
#include "godot_cpp/classes/shape3d.hpp"
#include "godot_cpp/classes/standard_material3d.hpp"


using namespace godot;


void VoxelTerrain::_bind_methods()
{
    BIND_VOXEL_NODE_PROPERTIES(VoxelTerrain);

    BIND_GODOT_PROPERTY_SETTER_AND_GETTERS(check_disconnected_components, VoxelTerrain)
	BIND_GODOT_PROPERTY_SETTER_AND_GETTERS(draw_toughness_debug_mesh, VoxelTerrain)

	BIND_GODOT_PROPERTY(FLOAT, noise_frequency, VoxelTerrain)
	BIND_GODOT_PROPERTY(FLOAT, toughness_noise_frequency, VoxelTerrain)
	BIND_GODOT_PROPERTY(FLOAT, noise_voxel_spawn_threshold, VoxelTerrain)

	BIND_GODOT_PROPERTY_WITH_ARGS(INT, noise_type, VoxelTerrain, PROPERTY_HINT_ENUM, "Simplex,Simplex Smooth, Cellular, Perlin, Value Cubic, Value")
	BIND_GODOT_PROPERTY_WITH_ARGS(INT, toughness_noise_type, VoxelTerrain, PROPERTY_HINT_ENUM, "Simplex,Simplex Smooth, Cellular, Perlin, Value Cubic, Value")

	ClassDB::bind_method(D_METHOD("create_noise_terrain_SVO"), &VoxelTerrain::create_noise_terrain_SVO);
	ClassDB::bind_method(D_METHOD("create_solid_cube_SVO"), &VoxelTerrain::create_solid_cube_SVO);
	ClassDB::bind_method(D_METHOD("initiate_fracture_propagation", "voxel_position", "initial_force"), &VoxelTerrain::initiate_fracture_propagation);
}


VoxelTerrain::VoxelTerrain() :
sparse_voxel_octree(terrain_chunk_size),
_noise_frequency(0.02f),
_noise_type(FastNoiseLite::NoiseType::TYPE_SIMPLEX),
_noise_voxel_spawn_threshold(0.2f)
{

}


VoxelTerrain::~VoxelTerrain() {

}


//////////////////
// Godot Functions


void VoxelTerrain::_ready()
{
    _initialise_nodes(this);

    terrain_noise_generator.instantiate();
    terrain_noise_generator->set_noise_type(_noise_type);
    terrain_noise_generator->set_frequency(_noise_frequency);

    toughness_noise_generator.instantiate();
    toughness_noise_generator->set_noise_type(_toughness_noise_type);
    toughness_noise_generator->set_frequency(_toughness_noise_frequency);
    toughness_noise_generator->set_seed(toughness_noise_generator->get_seed()+1.f);

    create_noise_terrain_SVO();
}


void VoxelTerrain::_process(double delta)
{
    _process_geometry_updates();
}


///////////////////
// Public Functions


void VoxelTerrain::remove_voxel(Vector3i voxel_position)
{
    //UtilityFunctions::print("VoxelTerrain remove_voxel()");
    sparse_voxel_octree.insert_voxel(voxel_position.x, voxel_position.y, voxel_position.z, 0);
}


VoxelUtility::Voxel VoxelTerrain::get_voxel(Vector3i voxel_position)
{
     return sparse_voxel_octree.get_voxel(voxel_position.x, voxel_position.y, voxel_position.z);
}


void VoxelTerrain::initiate_fracture_propagation(Vector3 voxel_position, float initial_force)
{
    /*
     * Steps:
     * - Calculate initial stress intensity, if higher than voxel toughness, begin propagation loop
     * - Check for disconnected voxels
     * - Regenerate mesh & collider
     */

    godot::UtilityFunctions::print("\nVoxelTerrain::initiate_fracture_propagation()");
    bool fracture_initiated = false;
    std::vector<VoxelUtility::FractureFront> fracture_front_voxels;

    // Check if fracture has enough force to initiate
    // Starting with a naive implementation
    float current_voxel_toughness = _get_voxel_toughness(voxel_position);
    godot::UtilityFunctions::print("VoxelToughness: ", _get_voxel_toughness(voxel_position)); // TEST
    float stress_intensity = initial_force;
    if (stress_intensity > current_voxel_toughness)
    {
        fracture_initiated = true;
        fracture_front_voxels.push_back({voxel_position, stress_intensity});
        remove_voxel(voxel_position);
    }

    // If impact has enough force to break the first voxel, then propagate fracture.
    if (fracture_initiated)
    {
        // Propagate Fracture
        std::chrono::time_point<std::chrono::steady_clock> start_time = std::chrono::steady_clock::now();
        while (!fracture_front_voxels.empty())
        {
            const VoxelUtility::FractureFront fracture_front = fracture_front_voxels.back();
            _propagate_fracture(fracture_front_voxels, fracture_front.voxel_position, fracture_front.stress_intensity);
        }
        std::chrono::time_point<std::chrono::steady_clock> end_time = std::chrono::steady_clock::now();
        double elapsed_time = std::chrono::duration<double, std::milli>(end_time - start_time).count();
        UtilityFunctions::print("VoxelTerrain fracture propagation took ", elapsed_time, " ms");
        // Create new voxel objects from disconnected voxels
        //_generate_bodies_from_disconnected_voxels(); // DISABLED FOR NOW SO I CAN SEE THE FRACTURE
        // Trigger mesh & collider regeneration
        _mesh_needs_generated = true;
        _collider_needs_generated = true;
    }
}


void VoxelTerrain::create_solid_cube_SVO()
{
    // Clear Previous Voxels
    sparse_voxel_octree.clear_voxels();

    // NOTE: This breaks if you output too many godot debug statements!
    std::chrono::time_point<std::chrono::steady_clock> start_time = std::chrono::steady_clock::now();
    for (int x = 0; x < terrain_chunk_size; x++)
    {
        for (int y = 0; y < terrain_chunk_size; y++)
        {
            for (int z = 0; z < terrain_chunk_size; z++)
            {
                sparse_voxel_octree.insert_voxel(x,y,z,1);
            }
        }
    }
    std::chrono::time_point<std::chrono::steady_clock> end_time = std::chrono::steady_clock::now();
    double elapsed_time = std::chrono::duration<double, std::milli>(end_time - start_time).count();
    UtilityFunctions::print("Terrain Solid Cube generation took: ", elapsed_time, " ms");
    UtilityFunctions::print("Terrain NumBytes: ", sparse_voxel_octree.get_total_bytes_in_memory());

    _voxel_grid_initialised = true;
}


void VoxelTerrain::create_noise_terrain_SVO()
{
    // Clear Previous Voxels
    sparse_voxel_octree.clear_voxels();

    // NOTE: This breaks if you output too many godot debug statements!
    std::chrono::time_point<std::chrono::steady_clock> start_time = std::chrono::steady_clock::now();
    for (int x = 0; x < terrain_chunk_size; x++)
    {
        for (int y = 0; y < terrain_chunk_size; y++)
        {
            for (int z = 0; z < terrain_chunk_size; z++)
            {
                float noise_value = terrain_noise_generator->get_noise_3d(x, y, z);
                if (noise_value > _noise_voxel_spawn_threshold)
                {
                    sparse_voxel_octree.insert_voxel(x,y,z,1);
                }
            }
        }
    }
    std::chrono::time_point<std::chrono::steady_clock> end_time = std::chrono::steady_clock::now();
    double elapsed_time = std::chrono::duration<double, std::milli>(end_time - start_time).count();
    UtilityFunctions::print("\nTerrain Noise generation took: ", elapsed_time, " ms");
    UtilityFunctions::print("Terrain NumBytes: ", sparse_voxel_octree.get_total_bytes_in_memory());

    _voxel_grid_initialised = true;
}


////////////////////
// Getters & Setters


void VoxelTerrain::set_noise_frequency(const float new_noise_frequency)
{
    _noise_frequency = new_noise_frequency;
    terrain_noise_generator->set_frequency(_noise_frequency);
}


void VoxelTerrain::set_toughness_noise_frequency(const float new_toughness_noise_frequency)
{
    _toughness_noise_frequency = new_toughness_noise_frequency;
    toughness_noise_generator->set_frequency(_toughness_noise_frequency);
}


void VoxelTerrain::set_noise_type(const FastNoiseLite::NoiseType new_noise_type)
{
    _noise_type = new_noise_type;
    terrain_noise_generator->set_noise_type(_noise_type);
}


void VoxelTerrain::set_toughness_noise_type(const FastNoiseLite::NoiseType new_toughness_noise_type)
{
    _toughness_noise_type = new_toughness_noise_type;
    toughness_noise_generator->set_noise_type(_toughness_noise_type);
}


////////////////////
// Private Functions


void VoxelTerrain::_morton_code_test()
{
    /*
     * Seems lib morton will place XYZ from LSB - MSB (so X is LSB and Z is MSB). Also the leftmost sequence of 3 bits is the starting point instead
     * of the 3 bits on the far right of the Morton code integer.
     */
    int64_t index1 = libmorton::morton3D_64_encode(63,63,63);
    // In a 2x2x2 grid I expect this to be right node (x axis) in a 1 depth octree
    // Expected Binary Output: 100
    int64_t index2 = libmorton::morton3D_64_encode(1,0,0);
    int64_t index3 = libmorton::morton3D_64_encode(0,1,0);
    int64_t index4 = libmorton::morton3D_64_encode(0,0,1);
    int64_t index5 = libmorton::morton3D_64_encode(1,1,1);
    // Depth 2, Right on X axis then left on X axis:
    // Result: 001, 000
    int64_t index6 = libmorton::morton3D_64_encode(2,0,0);
    // In a 4x4x4 grid, since values are 0-3, with 3 being the furthest point on that axis:
    // I expect this to be a depth of 2; From Root: Rightmost node on the x axis, From child of root: rightmost node on X. so 100,100
    // On the Z axis it would be rightmost node then leftmost. so 001, 000
    // Combined: 101, 001
    int64_t index7 = libmorton::morton3D_64_encode(3,0,2);
    // In order:
    // X: 001, 001, 001, 001
    // Y: 010, 010, 010, 000
    // Z: 000, 000, 100, 100
    // Combined: 011, 011, 111, 101
    // 3, 3, 7, 5
    int64_t index8 = libmorton::morton3D_64_encode(15,14,3);
    UtilityFunctions::print("\nIndex8: ", std::bitset<64>(index8).to_string().c_str());
    for (int i = 3 * (4-1); i >= 0; i-=3)
    {
        uint_fast16_t node_index = (index8 >> i) & 0b111;
         UtilityFunctions::print("Index8 sequence:", std::bitset<64>(node_index).to_string().c_str());
    }
    sparse_voxel_octree.insert_voxel(15, 14, 3, 1);
    //sparse_voxel_octree.insert_voxel(15, 14, 3, 1);
    //sparse_voxel_octree.insert_voxel(15, 14, 3, 1);
    sparse_voxel_octree.get_voxel(15,14,3);
    sparse_voxel_octree.get_voxel(14,14,13);
    sparse_voxel_octree.get_voxel(0,0,0);
    //UtilityFunctions::print("Value found in octree at 15, 14, 3: ", sparse_voxel_octree.get_voxel(15,14,13));
    //UtilityFunctions::print("Value found in octree at 14, 14, 3: ", sparse_voxel_octree.get_voxel(14,14,13));
    //UtilityFunctions::print("Value found in octree at 0, 0, 0: ", sparse_voxel_octree.get_voxel(0,0,0));

    int64_t index9 = libmorton::morton3D_64_encode(127,127,127);
    int64_t index10 = libmorton::morton3D_64_encode(6, 8, 9);
    int64_t index11 = libmorton::morton3D_64_encode(6, 5, 7);

    UtilityFunctions::print("\nMorton Code 1 : ", std::bitset<64>(index1).to_string().c_str(), " int: ", index1);
    UtilityFunctions::print("Morton Code 2: ", std::bitset<64>(index2).to_string().c_str(), " int: ", index2);
    UtilityFunctions::print("Morton Code 3: ", std::bitset<64>(index3).to_string().c_str(), " int: ", index3);
    UtilityFunctions::print("Morton Code 4: ", std::bitset<64>(index4).to_string().c_str(), " int: ", index4);
    UtilityFunctions::print("Morton Code 5: ", std::bitset<64>(index5).to_string().c_str(), " int: ", index5);
    UtilityFunctions::print("Morton Code 6: ", std::bitset<64>(index6).to_string().c_str(), " int: ", index6);
    UtilityFunctions::print("Morton Code 7: ", std::bitset<64>(index7).to_string().c_str(), " int: ", index7);
    UtilityFunctions::print("Morton Code 8: ", std::bitset<64>(index8).to_string().c_str(), " int: ", index8);
    UtilityFunctions::print("Morton Code 9: ", std::bitset<64>(index9).to_string().c_str(), " int: ", index9);
    UtilityFunctions::print("Morton Code 10: ", std::bitset<64>(index10).to_string().c_str(), " int: ", index10);
    UtilityFunctions::print("Morton Code 11: ", std::bitset<64>(index11).to_string().c_str(), " int: ", index11);
}


void VoxelTerrain::_process_geometry_updates()
{
    VoxelNode::_process_geometry_updates();
    if (_check_disconnected_components)
    {
        _generate_bodies_from_disconnected_voxels();
        _check_disconnected_components = false;
    }
}


void VoxelTerrain::_generate_mesh()
{
    if (!_mesh_node || !_mesh_needs_generated) { return; }

    /// Procedurally generate voxel mesh from voxel grid

    UtilityFunctions::print("\nGenerating VoxelTerrain Mesh");
    _timings.clear();
    for (int i = 0; i < _num_mesh_generation_tests_to_run; i++)
    {
        std::chrono::time_point<std::chrono::steady_clock> start_time = std::chrono::steady_clock::now();

        if (_draw_toughness_debug_mesh)
        {
            _generate_toughness_debug_mesh();
        }
        else
        {
            _generate_culling_voxel_mesh();
        }

        std::chrono::time_point<std::chrono::steady_clock> end_time = std::chrono::steady_clock::now();
        double elapsed_time = std::chrono::duration<double, std::milli>(end_time - start_time).count();
        _timings.push_back(elapsed_time);
        // To prevent overflow of the debug output, don't output for every test if testing a lot of iterations.
        if (_num_mesh_generation_tests_to_run < 500)
        {
            UtilityFunctions::print("VoxelTerrain mesh generation took ", elapsed_time, " ms");
        }
    }
    double average_time = 0.0;
    for (auto time : _timings)
    {
        average_time += time;
    }
    average_time /= _timings.size();
    UtilityFunctions::print("VoxelTerrain mesh generation took an average of ", average_time, " ms");

    _mesh_needs_generated = false;
    _collider_needs_generated = true;
}


void VoxelTerrain::_generate_culling_voxel_mesh()
{
    /*
     * TODO: Make this take advantage of the octree so that it only loops through existing voxels instead of checking every voxel!
     * I can check for the existing voxels via traversal of the octree (Although maybe this is actually slower, this is something to test)
     */

    if (!_mesh_node) { return;}

    //// VERTICES

    std::vector<Vector3> vertices_vector;
    PackedVector3Array vertices;
    std::vector<int32_t> indices_vector;
    PackedInt32Array indices;
    std::vector<Vector3> normals_vector;
    PackedVector3Array normals;
    // std::vector<Color> colours_vector;
    // PackedColorArray colours;
    // std::vector<Vector2> uvs_vector;
    // PackedVector2Array uvs;

    /// Generate vertices per voxel

    int vertex_offset = 0;
    for (int x = 0; x < terrain_chunk_size; x++)
    {
        for (int y = 0; y < terrain_chunk_size; y++)
        {
            for (int z = 0; z < terrain_chunk_size; z++)
            {
                /// Ignore empty voxels

                if (sparse_voxel_octree.get_voxel(x,y,z) == 0) { continue; }

                //// Render voxel faces
                // Only render faces if the voxel adjacent to that face is empty, otherwise do not draw it (cull the face).

                /// Construct vertex positions (there are 8)
                // All faces drawn in Z shape, starting from top left.
                // x -+: left-right
                // y -+: bottom-top
                // z -+: front-back

                const Vector3 back_top_left = Vector3(-0.5 + x, 0.5 + y, 0.5 + z);
                const Vector3 back_top_right = Vector3(0.5 + x, 0.5 + y, 0.5 + z);
                const Vector3 back_bottom_left = Vector3(-0.5 + x, -0.5 + y, 0.5 + z);
                const Vector3 back_bottom_right = Vector3(0.5 + x, -0.5 + y, 0.5 + z);
                const Vector3 front_top_left = Vector3(-0.5 + x, 0.5 + y, -0.5 + z);
                const Vector3 front_top_right = Vector3(0.5 + x, 0.5 + y, -0.5 + z);
                const Vector3 front_bottom_left = Vector3(-0.5 + x, -0.5 + y, -0.5 + z);
                const Vector3 front_bottom_right = Vector3(0.5 + x, -0.5 + y, -0.5 + z);

                /// TOP FACE

                uint8_t voxel_value = 0;
                if (y+1 < terrain_chunk_size) { voxel_value = sparse_voxel_octree.get_voxel(x,y + 1,z); };
                if (voxel_value == 0)
                {
                    vertices_vector.push_back(front_top_left);
                    vertices_vector.push_back(front_top_right);
                    vertices_vector.push_back(back_top_left);
                    vertices_vector.push_back(back_top_right);

                    VoxelUtility::generate_indices_for_quad(indices_vector, vertex_offset);
                    VoxelUtility::generate_normals_for_quad(normals_vector, VoxelUtility::UP);

                    vertex_offset += 4;
                }

                /// BOTTOM FACE

                voxel_value = 0;
                if (y-1 >= 0) { voxel_value = sparse_voxel_octree.get_voxel(x,y - 1,z); };
                if (voxel_value == 0)
                {
                    vertices_vector.push_back(back_bottom_left);
                    vertices_vector.push_back(back_bottom_right);
                    vertices_vector.push_back(front_bottom_left);
                    vertices_vector.push_back(front_bottom_right);

                    VoxelUtility::generate_indices_for_quad(indices_vector, vertex_offset);
                    VoxelUtility::generate_normals_for_quad(normals_vector, VoxelUtility::DOWN);

                    vertex_offset += 4;
                }

                /// LEFT FACE

                voxel_value = 0;
                if (x-1 >= 0) { voxel_value = sparse_voxel_octree.get_voxel(x - 1,y,z); };
                if (voxel_value == 0)
                {
                    vertices_vector.push_back(front_top_left);
                    vertices_vector.push_back(back_top_left);
                    vertices_vector.push_back(front_bottom_left);
                    vertices_vector.push_back(back_bottom_left);

                    VoxelUtility::generate_indices_for_quad(indices_vector, vertex_offset);
                    VoxelUtility::generate_normals_for_quad(normals_vector, VoxelUtility::LEFT);

                    vertex_offset += 4;
                }

                /// RIGHT FACE

                voxel_value = 0;
                if (x+1 < terrain_chunk_size) { voxel_value = sparse_voxel_octree.get_voxel(x + 1,y,z); };
                if (voxel_value == 0)
                {
                    vertices_vector.push_back(back_top_right);
                    vertices_vector.push_back(front_top_right);
                    vertices_vector.push_back(back_bottom_right);
                    vertices_vector.push_back(front_bottom_right);

                    VoxelUtility::generate_indices_for_quad(indices_vector, vertex_offset);
                    VoxelUtility::generate_normals_for_quad(normals_vector, VoxelUtility::RIGHT);

                    vertex_offset += 4;
                }

                /// FRONT FACE

                voxel_value = 0;
                if (z-1 >= 0) { voxel_value = sparse_voxel_octree.get_voxel(x,y,z-1); };
                if (voxel_value == 0)
                {
                    vertices_vector.push_back(front_top_right);
                    vertices_vector.push_back(front_top_left);
                    vertices_vector.push_back(front_bottom_right);
                    vertices_vector.push_back(front_bottom_left);

                    VoxelUtility::generate_indices_for_quad(indices_vector, vertex_offset);
                    VoxelUtility::generate_normals_for_quad(normals_vector, VoxelUtility::FORWARD);

                    vertex_offset += 4;
                }

                /// BACK FACE

                voxel_value = 0;
                if (z+1 < terrain_chunk_size) { voxel_value = sparse_voxel_octree.get_voxel(x,y,z+1); };
                if (voxel_value == 0)
                {
                    vertices_vector.push_back(back_top_left);
                    vertices_vector.push_back(back_top_right);
                    vertices_vector.push_back(back_bottom_left);
                    vertices_vector.push_back(back_bottom_right);

                    VoxelUtility::generate_indices_for_quad(indices_vector, vertex_offset);
                    VoxelUtility::generate_normals_for_quad(normals_vector, VoxelUtility::BACKWARD);

                    vertex_offset += 4;
                }
            }
        }
    }

    // Since the voxel grid is empty don't create mesh and remove previous mesh
    if (vertices_vector.size() <= 0)
    {
        Ref<Mesh> mesh = _mesh_node->get_mesh();
        if(ArrayMesh* array_mesh = Object::cast_to<ArrayMesh>(mesh.ptr()))
        {
            array_mesh->clear_surfaces();
        }
        return;
    }

    /// Pack data into arrays

    vertices.resize(vertices_vector.size());
    for (int i = 0; i < vertices_vector.size(); i ++)
    {
        vertices.set(i, vertices_vector[i]);
    }
    indices.resize(indices_vector.size());
    for (int i = 0; i < indices_vector.size(); i ++)
    {
        indices.set(i, indices_vector[i]);
    }
    normals.resize(normals_vector.size());
    for (int i = 0; i < normals_vector.size(); i ++)
    {
        normals.set(i, normals_vector[i]);
    }
    // uvs.resize(uvs_vector.size());
    // for (int i = 0; i < uvs_vector.size(); i ++)
    // {
    //     uvs.set(i, uvs_vector[i]);
    // }

    /// Create surface array from generated mesh data

    Array surface_array;
    surface_array.resize(Mesh::ArrayType::ARRAY_MAX);
    surface_array[(int)Mesh::ArrayType::ARRAY_VERTEX] = vertices;
    surface_array[(int)Mesh::ArrayType::ARRAY_INDEX] = indices;
    surface_array[(int)Mesh::ArrayType::ARRAY_NORMAL] = normals;
    //surface_array[(int)Mesh::ArrayType::ARRAY_COLOR] = colours;
    //surface_array[(int)Mesh::ArrayType::ARRAY_TEX_UV] = uvs;

    /// Create mesh from Surface Array

    Ref<Mesh> mesh = _mesh_node->get_mesh();
    if(ArrayMesh* array_mesh = Object::cast_to<ArrayMesh>(mesh.ptr()))
    {
        array_mesh->clear_surfaces();
        array_mesh->add_surface_from_arrays(Mesh::PrimitiveType::PRIMITIVE_TRIANGLES, surface_array);
    }
}


void VoxelTerrain::_generate_toughness_debug_mesh()
{
    /*
     * TODO: Make this take advantage of the octree so that it only loops through existing voxels instead of checking every voxel!
     * I can check for the existing voxels via traversal of the octree (Although maybe this is actually slower, this is something to test)
     */

    if (!_mesh_node) { return;}

    //// VERTICES

    std::vector<Vector3> vertices_vector;
    PackedVector3Array vertices;
    std::vector<int32_t> indices_vector;
    PackedInt32Array indices;
    std::vector<Vector3> normals_vector;
    PackedVector3Array normals;
    std::vector<Color> colours_vector;
    PackedColorArray colours;

    /// Generate vertices per voxel

    int vertex_offset = 0;
    for (int x = 0; x < terrain_chunk_size; x++)
    {
        for (int y = 0; y < terrain_chunk_size; y++)
        {
            for (int z = 0; z < terrain_chunk_size; z++)
            {
                /// Ignore empty voxels

                if (sparse_voxel_octree.get_voxel(x,y,z) == 0) { continue; }

                //// Render voxel faces
                // Only render faces if the voxel adjacent to that face is empty, otherwise do not draw it (cull the face).

                /// Construct vertex positions (there are 8)
                // All faces drawn in Z shape, starting from top left.
                // x -+: left-right
                // y -+: bottom-top
                // z -+: front-back

                const Vector3 back_top_left = Vector3(-0.5 + x, 0.5 + y, 0.5 + z);
                const Vector3 back_top_right = Vector3(0.5 + x, 0.5 + y, 0.5 + z);
                const Vector3 back_bottom_left = Vector3(-0.5 + x, -0.5 + y, 0.5 + z);
                const Vector3 back_bottom_right = Vector3(0.5 + x, -0.5 + y, 0.5 + z);
                const Vector3 front_top_left = Vector3(-0.5 + x, 0.5 + y, -0.5 + z);
                const Vector3 front_top_right = Vector3(0.5 + x, 0.5 + y, -0.5 + z);
                const Vector3 front_bottom_left = Vector3(-0.5 + x, -0.5 + y, -0.5 + z);
                const Vector3 front_bottom_right = Vector3(0.5 + x, -0.5 + y, -0.5 + z);

                /// TOUGHNESS COLOUR CALCULATION

                // Render between 2 colours to represent the variation between min and max toughness
                float toughness = _get_voxel_toughness(Vector3i(x,y,z));
                // between 0 and 1 from minimum toughness to maximum toughness
                float toughness_factor = (toughness-min_toughness) / (max_toughness-min_toughness);
                toughness_factor = std::clamp(toughness_factor, 0.f, 1.f);
                godot::Color voxel_colour = godot::Color(toughness_factor, 1.0f-toughness_factor, 0.f, 1.f);

                /// TOP FACE

                uint8_t voxel_value = 0;
                if (y+1 < terrain_chunk_size) { voxel_value = sparse_voxel_octree.get_voxel(x,y + 1,z); };
                if (voxel_value == 0)
                {
                    vertices_vector.push_back(front_top_left);
                    vertices_vector.push_back(front_top_right);
                    vertices_vector.push_back(back_top_left);
                    vertices_vector.push_back(back_top_right);

                    VoxelUtility::generate_colours_for_quad(colours_vector, voxel_colour);
                    VoxelUtility::generate_indices_for_quad(indices_vector, vertex_offset);
                    VoxelUtility::generate_normals_for_quad(normals_vector, VoxelUtility::UP);

                    vertex_offset += 4;
                }

                /// BOTTOM FACE

                voxel_value = 0;
                if (y-1 >= 0) { voxel_value = sparse_voxel_octree.get_voxel(x,y - 1,z); };
                if (voxel_value == 0)
                {
                    vertices_vector.push_back(back_bottom_left);
                    vertices_vector.push_back(back_bottom_right);
                    vertices_vector.push_back(front_bottom_left);
                    vertices_vector.push_back(front_bottom_right);

                    VoxelUtility::generate_colours_for_quad(colours_vector, voxel_colour);
                    VoxelUtility::generate_indices_for_quad(indices_vector, vertex_offset);
                    VoxelUtility::generate_normals_for_quad(normals_vector, VoxelUtility::DOWN);

                    vertex_offset += 4;
                }

                /// LEFT FACE

                voxel_value = 0;
                if (x-1 >= 0) { voxel_value = sparse_voxel_octree.get_voxel(x - 1,y,z); };
                if (voxel_value == 0)
                {
                    vertices_vector.push_back(front_top_left);
                    vertices_vector.push_back(back_top_left);
                    vertices_vector.push_back(front_bottom_left);
                    vertices_vector.push_back(back_bottom_left);

                    VoxelUtility::generate_colours_for_quad(colours_vector, voxel_colour);
                    VoxelUtility::generate_indices_for_quad(indices_vector, vertex_offset);
                    VoxelUtility::generate_normals_for_quad(normals_vector, VoxelUtility::LEFT);

                    vertex_offset += 4;
                }

                /// RIGHT FACE

                voxel_value = 0;
                if (x+1 < terrain_chunk_size) { voxel_value = sparse_voxel_octree.get_voxel(x + 1,y,z); };
                if (voxel_value == 0)
                {
                    vertices_vector.push_back(back_top_right);
                    vertices_vector.push_back(front_top_right);
                    vertices_vector.push_back(back_bottom_right);
                    vertices_vector.push_back(front_bottom_right);

                    VoxelUtility::generate_colours_for_quad(colours_vector, voxel_colour);
                    VoxelUtility::generate_indices_for_quad(indices_vector, vertex_offset);
                    VoxelUtility::generate_normals_for_quad(normals_vector, VoxelUtility::RIGHT);

                    vertex_offset += 4;
                }

                /// FRONT FACE

                voxel_value = 0;
                if (z-1 >= 0) { voxel_value = sparse_voxel_octree.get_voxel(x,y,z-1); };
                if (voxel_value == 0)
                {
                    vertices_vector.push_back(front_top_right);
                    vertices_vector.push_back(front_top_left);
                    vertices_vector.push_back(front_bottom_right);
                    vertices_vector.push_back(front_bottom_left);

                    VoxelUtility::generate_colours_for_quad(colours_vector, voxel_colour);
                    VoxelUtility::generate_indices_for_quad(indices_vector, vertex_offset);
                    VoxelUtility::generate_normals_for_quad(normals_vector, VoxelUtility::FORWARD);

                    vertex_offset += 4;
                }

                /// BACK FACE

                voxel_value = 0;
                if (z+1 < terrain_chunk_size) { voxel_value = sparse_voxel_octree.get_voxel(x,y,z+1); };
                if (voxel_value == 0)
                {
                    vertices_vector.push_back(back_top_left);
                    vertices_vector.push_back(back_top_right);
                    vertices_vector.push_back(back_bottom_left);
                    vertices_vector.push_back(back_bottom_right);

                    VoxelUtility::generate_colours_for_quad(colours_vector, voxel_colour);
                    VoxelUtility::generate_indices_for_quad(indices_vector, vertex_offset);
                    VoxelUtility::generate_normals_for_quad(normals_vector, VoxelUtility::BACKWARD);

                    vertex_offset += 4;
                }
            }
        }
    }

    // Since the voxel grid is empty don't create mesh and remove previous mesh
    if (vertices_vector.size() <= 0)
    {
        Ref<Mesh> mesh = _mesh_node->get_mesh();
        if(ArrayMesh* array_mesh = Object::cast_to<ArrayMesh>(mesh.ptr()))
        {
            array_mesh->clear_surfaces();
        }
        return;
    }

    /// Pack data into arrays

    vertices.resize(vertices_vector.size());
    for (int i = 0; i < vertices_vector.size(); i ++)
    {
        vertices.set(i, vertices_vector[i]);
    }
    indices.resize(indices_vector.size());
    for (int i = 0; i < indices_vector.size(); i ++)
    {
        indices.set(i, indices_vector[i]);
    }
    normals.resize(normals_vector.size());
    for (int i = 0; i < normals_vector.size(); i ++)
    {
        normals.set(i, normals_vector[i]);
    }
    colours.resize(colours_vector.size());
    for (int i = 0; i < colours_vector.size(); i ++)
    {
        colours.set(i, colours_vector[i]);
    }

    /// Create surface array from generated mesh data

    Array surface_array;
    surface_array.resize(Mesh::ArrayType::ARRAY_MAX);
    surface_array[(int)Mesh::ArrayType::ARRAY_VERTEX] = vertices;
    surface_array[(int)Mesh::ArrayType::ARRAY_INDEX] = indices;
    surface_array[(int)Mesh::ArrayType::ARRAY_NORMAL] = normals;
    surface_array[(int)Mesh::ArrayType::ARRAY_COLOR] = colours;

    /// Create mesh from Surface Array

    Ref<Mesh> mesh = _mesh_node->get_mesh();
    if(ArrayMesh* array_mesh = Object::cast_to<ArrayMesh>(mesh.ptr()))
    {
        array_mesh->clear_surfaces();
        array_mesh->add_surface_from_arrays(Mesh::PrimitiveType::PRIMITIVE_TRIANGLES, surface_array);

        // Set material so colours can be used in the mesh
        Ref<StandardMaterial3D> material;
        material.instantiate();
        material->set_flag(godot::BaseMaterial3D::FLAG_ALBEDO_FROM_VERTEX_COLOR, true);
        array_mesh->surface_set_material(0, material);
    }
}


void VoxelTerrain::_generate_collider()
{
    // TODO: Use the collision node I generate on _ready() and procedurally generate the mesh myself
    // Can hopefully use the same algorithm as mesh generation and that may be way faster than trimesh generation done by godot.

    // Don't generate collider if there is no mesh (And therefore no voxel grid)
    if (!_mesh_node || !_mesh_node->get_mesh().ptr() || _mesh_node->get_mesh()->get_surface_count() == 0)
    {
        if (_collider_node)
        {
            // Assign an empty collider
            Ref<Shape3D> empty_collider;
            _collider_node->set_shape(empty_collider);
        }
        return;
    }

    std::chrono::time_point<std::chrono::steady_clock> start_time = std::chrono::steady_clock::now();

    // Note This seems to create a static body child, I need it only create a collider and under the same parent
    _mesh_node->create_convex_collision();
    //_mesh_node->create_trimesh_collision();

    // Temp fix
    if (_collider_node) {
        _collider_node->queue_free();
        _collider_node = nullptr;
    }
    _collider_node = dynamic_cast<CollisionShape3D*>(_mesh_node->get_child(0)->get_child(0));
    _collider_node->reparent(this);
    _mesh_node->get_child(0)->queue_free(); // Remove the static body

    std::chrono::time_point<std::chrono::steady_clock> end_time = std::chrono::steady_clock::now();
    double elapsed_time = std::chrono::duration<double, std::milli>(end_time - start_time).count();
    UtilityFunctions::print("VoxelTerrain collider generation took ", elapsed_time, " ms");
}


void VoxelTerrain::_propagate_fracture(std::vector<VoxelUtility::FractureFront>& fracture_fronts,  Vector3i voxel_position, float stress_intensity)
{
    std::vector<std::pair<float, Vector3i>> surrounding_voxel_toughness;
    // Check 6 surrounding voxels.
    // Really naive solution just to see some sort of fracturing.
    if (voxel_position.x < terrain_chunk_size)
    {
        const Vector3i offset_1 = voxel_position + Vector3i(1,0,0);
        surrounding_voxel_toughness.push_back({_get_voxel_toughness(offset_1), offset_1});
    }
    if (voxel_position.x > 0)
    {
        const Vector3i offset_2 = voxel_position - Vector3i(1,0,0);
        surrounding_voxel_toughness.push_back({_get_voxel_toughness(offset_2), offset_2});
    }
    if (voxel_position.y < terrain_chunk_size)
    {
        const Vector3i offset_3 = voxel_position + Vector3i(0,1,0);
        surrounding_voxel_toughness.push_back({_get_voxel_toughness(offset_3), offset_3});
    }
    if (voxel_position.y > 0)
    {
        const Vector3i offset_4 = voxel_position - Vector3i(0,1,0);
        surrounding_voxel_toughness.push_back({_get_voxel_toughness(offset_4), offset_4});
    }
    if (voxel_position.z < terrain_chunk_size)
    {
        const Vector3i offset_5 = voxel_position + Vector3i(0,0,1);
        surrounding_voxel_toughness.push_back({_get_voxel_toughness(offset_5), offset_5});
    }
    if (voxel_position.z > 0)
    {
        const Vector3i offset_6 = voxel_position - Vector3i(0,0,1);
        surrounding_voxel_toughness.push_back({_get_voxel_toughness(offset_6), offset_6});
    }

    // Sort so lowest toughness first
    std::sort(surrounding_voxel_toughness.begin(), surrounding_voxel_toughness.end(),
        [](const auto& left, const auto& right) {
            return left.first < right.first;
        });
    // DEBUG
    //godot::UtilityFunctions::print("SurroundingVoxelToughness: ", surrounding_voxel_toughness[0].first, " ",
    //    surrounding_voxel_toughness[1].first, " ",  surrounding_voxel_toughness[2].first);
    const Vector3i& voxel_to_propagate_to = surrounding_voxel_toughness[0].second;
    if (stress_intensity > surrounding_voxel_toughness[0].first &&
        sparse_voxel_octree.get_voxel(voxel_to_propagate_to.x, voxel_to_propagate_to.y, voxel_to_propagate_to.z) != 0)
    {
        fracture_fronts.pop_back();
        remove_voxel(voxel_position);
        fracture_fronts.push_back({surrounding_voxel_toughness[0].second, stress_intensity});
    }
    else {
        fracture_fronts.pop_back();
    }
}


float VoxelTerrain::_get_voxel_toughness(Vector3i voxel_position)
{
    // Expected output of -1 to 1 that is spatially varied
    float noise_value = toughness_noise_generator->get_noise_3d(voxel_position.x, voxel_position.y, voxel_position.z);
    if (_toughness_noise_type != FastNoiseLite::NoiseType::TYPE_CELLULAR)
    {
        // Convert from (-1 to 1) to (0 to 1)
        noise_value += 1;
        noise_value /= 2;
    }
    else
    {
        // Convert from (-1 to 0) to (0 to 1)
        noise_value += 1;
    }

    float toughness = 0.f;
    // Interpolate between a minimum and maximum toughness based on the noise value
    toughness = min_toughness + noise_value * (max_toughness - min_toughness);
    return toughness;
}


void VoxelTerrain::_generate_bodies_from_disconnected_voxels()
{
    std::chrono::time_point<std::chrono::steady_clock> start_time = std::chrono::steady_clock::now();

    // Find and remove the disconnected voxels from the terrain's voxel grid
    std::vector<VoxelUtility::VoxelGrid> disconnected_voxel_grids = _extract_disconnected_voxel_groups();

    // Create a new RigidBody Voxel Grid in the scene for each disconnected voxel grid.
    // Position it relative to the terrain chunk where the voxel grid originated from.
    for (const auto& [voxel_vector, dimensions, grid_offset] : disconnected_voxel_grids)
    {
        VoxelObject* new_voxel_object = memnew(VoxelObject);
        new_voxel_object->initialise(dimensions, voxel_vector);
        get_parent()->add_child(new_voxel_object);
        // Make sure to apply scale before setting the global position or they will not match at scales other than 1!
        new_voxel_object->set_scale(get_scale());
        new_voxel_object->set_global_position(get_global_position() + (get_scale() * grid_offset));
    }

    std::chrono::time_point<std::chrono::steady_clock> end_time = std::chrono::steady_clock::now();
    double elapsed_time = std::chrono::duration<double, std::milli>(end_time - start_time).count();
    UtilityFunctions::print("\nVoxelTerrain connected-component scan and body generation took ", elapsed_time, " ms");

    _mesh_needs_generated = true;
}


std::vector<VoxelUtility::VoxelGrid> VoxelTerrain::_extract_disconnected_voxel_groups()
{
    return _flood_filled_connected_component_voxel_extraction();
}


std::vector<VoxelUtility::VoxelGrid> VoxelTerrain::_flood_filled_connected_component_voxel_extraction()
{
    /*
     * Flood-Filled Connected Component Algorithm is used, with 6 connectivity (Equivalent to 4 connectivity in 2D).
     * Chosen for its simplicity.
     * Only immediately adjacent voxels are checked instead of voxels on the diagonals.
     * The entire chunk will be scanned.
     */
    /* IDEA:
     * Stored a 1D boolean array of size terrain_chunk_size^3 to represent whether a voxel has been visited by the flood fill algorithm.
     * Main loop that scans over every voxel in the grid and checks if it isn't empty (!= 0) and hasn't been visited yet.
     * If both conditions are true then a new extracted grid is created and a flood fill recursive function is called on
     * that voxel.
     *
     * Flood fill function will first do a boundary check to check voxel isn't outside of the grid.
     * Then the voxel will be evaluated for whether it is not empty and hasn't been visited yet.
     * If both conditions are true, then it will be marked as visited, placed into the newly created grid
     * and finally removed from the terrain chunk.
     * Then the function will scan in the 6 orthogonal directions, calling the flood fill function at each adjacent voxel.
     *
     * Each voxel that is extracted will explicitly store its position until the full bounds of their respective grids
     * has been obtained / flood fill is complete. You can't place them into the actual grid until then as you need the
     * grid bounds to index the voxel grid vector.
     */

    UtilityFunctions::print("\nFlood fill connected component start");
    std::vector<VoxelUtility::VoxelGrid> extracted_voxel_grids;
    // Store voxels with their position and voxel value in a separate vector as the full grid size
    // isn't known until the flood fill is complete
    std::vector<std::vector<VoxelUtility::ExtractedVoxel>> extracted_voxels;
    std::vector<bool> visited_voxels(terrain_chunk_size*terrain_chunk_size*terrain_chunk_size, false);
    // To mitigate stack overflow, instead of recursively flood filling, use a vector of voxel positions
    // and loop through each one individually.
    std::vector<std::tuple<int,int,int>> voxels_to_scan;

    for (int x = 0; x < terrain_chunk_size; x++)
    {
        for (int y = 0; y < terrain_chunk_size; y++)
        {
            for (int z = 0; z < terrain_chunk_size; z++)
            {
                // If voxel is not empty and hasn't been visited yet, then this is a new grid that can be scanned recursively
                int voxel_index = VoxelUtility::voxel_index_from_position(terrain_chunk_size, terrain_chunk_size, Vector3(x,y,z));
                if (sparse_voxel_octree.get_voxel(x, y, z) != 0 &&
                    visited_voxels[voxel_index] == false)
                {
                    // Create a new voxel grid and recursively scan from the voxel until the entire group has been found.
                    // Need highest and lowest position voxel so that grid can be correctly sized and to obtain the grid offset.

                    Vector3i grid_min_corner = Vector3i(x,y,z);
                    Vector3i grid_max_corner = Vector3i(x,y,z);
                    extracted_voxel_grids.emplace_back();
                    extracted_voxels.emplace_back();
                    voxels_to_scan.emplace_back(x,y,z);
                    while (!voxels_to_scan.empty())
                    {
                        auto [scan_x,scan_y,scan_z] = voxels_to_scan.back();
                        voxels_to_scan.pop_back();
                        _flood_fill(scan_x, scan_y, scan_z, visited_voxels, extracted_voxel_grids.back(),
                            extracted_voxels.back(), grid_min_corner, grid_max_corner, voxels_to_scan);
                    }
                    extracted_voxel_grids.back().offset_in_parent_grid = grid_min_corner;
                    extracted_voxel_grids.back().dimensions = grid_max_corner - grid_min_corner + Vector3i(1, 1, 1);
                }
            }
        }
    }

    // Construct the voxel grids from the extracted voxels

    for (int i = 0; i < extracted_voxel_grids.size(); i++)
    {
        const int width = extracted_voxel_grids[i].dimensions.x;
        const int height = extracted_voxel_grids[i].dimensions.y;
        const int depth = extracted_voxel_grids[i].dimensions.z;
        const int voxel_vector_size = width * height * depth;
        extracted_voxel_grids[i].voxels.resize(voxel_vector_size);
        for (auto& extracted_voxel : extracted_voxels[i])
        {
            const Vector3i new_voxel_position = extracted_voxel.position - extracted_voxel_grids[i].offset_in_parent_grid;
            const int voxel_index = VoxelUtility::voxel_index_from_position(width, height, new_voxel_position);
            extracted_voxel_grids[i].voxels[voxel_index] = extracted_voxel.voxel;
        }
    }

    return extracted_voxel_grids;
}


void VoxelTerrain::_flood_fill(const int x, const int y, const int z, std::vector<bool>& visited_voxels,
    VoxelUtility::VoxelGrid& voxel_grid, std::vector<VoxelUtility::ExtractedVoxel>& extracted_voxels,
    Vector3i& grid_min_corner, Vector3i& grid_max_corner, std::vector<std::tuple<int,int,int>>& voxels_to_scan)
{
    // Boundary check, voxel must be within the parent grid.
    if (x < 0 || y < 0 || z < 0 || x >= terrain_chunk_size || y >= terrain_chunk_size || z >= terrain_chunk_size) { return; }

    int voxel_index = VoxelUtility::voxel_index_from_position(terrain_chunk_size, terrain_chunk_size, Vector3(x,y,z));
    VoxelUtility::Voxel voxel_value = sparse_voxel_octree.get_voxel(x, y, z);
    // Check if voxel exists and is connected to the grid
    if (voxel_value != 0 &&
        visited_voxels[voxel_index] == false)
    {
        // Flag voxel as visited
        visited_voxels[voxel_index] = true;

        // Extract voxel
        extracted_voxels.push_back(VoxelUtility::ExtractedVoxel{godot::Vector3i(x,y,z), voxel_value});
        remove_voxel(Vector3(x,y,z));

        // Update extracted grids min and max bounds.
        grid_max_corner.x = std::max(x, grid_max_corner.x);
        grid_max_corner.y = std::max(y, grid_max_corner.y);
        grid_max_corner.z = std::max(z, grid_max_corner.z);
        grid_min_corner.x = std::min(x, grid_min_corner.x);
        grid_min_corner.y = std::min(y, grid_min_corner.y);
        grid_min_corner.z = std::min(z, grid_min_corner.z);

        // Scan in all orthogonal directions
        voxels_to_scan.emplace_back(x-1, y, z);
        voxels_to_scan.emplace_back(x+1, y, z);
        voxels_to_scan.emplace_back(x, y-1, z);
        voxels_to_scan.emplace_back(x, y+1, z);
        voxels_to_scan.emplace_back(x, y, z-1);
        voxels_to_scan.emplace_back(x, y, z+1);
    }
}
