#include "voxel_terrain.h"
#include "godot_cpp/classes/fast_noise_lite.hpp"
#include "godot_cpp/classes/global_constants.hpp"
#include <libmorton/morton.h>
#include <bitset>
#include "godot_cpp/variant/utility_functions.hpp"
#include "voxel_object.h"
#include "voxel_utility.h"


using namespace godot;


void VoxelTerrain::_bind_methods()
{
    BIND_VOXEL_NODE_PROPERTIES(VoxelTerrain);

	ClassDB::bind_method(D_METHOD("create_noise_terrain_SVO"), &VoxelTerrain::create_noise_terrain_SVO);
	ClassDB::bind_method(D_METHOD("create_solid_cube_SVO"), &VoxelTerrain::create_solid_cube_SVO);

	ClassDB::bind_method(D_METHOD("get_check_disconnected_components"), &VoxelTerrain::get_check_disconnected_components);
	ClassDB::bind_method(D_METHOD("set_check_disconnected_components", "new_check_disconnected_components"), &VoxelTerrain::set_check_disconnected_components);

	ClassDB::bind_method(D_METHOD("get_noise_frequency"), &VoxelTerrain::get_noise_frequency);
	ClassDB::bind_method(D_METHOD("set_noise_frequency", "new_noise_frequency"), &VoxelTerrain::set_noise_frequency);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "noise_frequency"), "set_noise_frequency", "get_noise_frequency");

	ClassDB::bind_method(D_METHOD("get_noise_type"), &VoxelTerrain::get_noise_type);
	ClassDB::bind_method(D_METHOD("set_noise_type", "new_noise_type"), &VoxelTerrain::set_noise_type);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "noise_type", PROPERTY_HINT_ENUM, "Simplex,Simplex Smooth, Cellular, Perlin, Value Cubic, Value"),
	"set_noise_type", "get_noise_type");

	ClassDB::bind_method(D_METHOD("get_noise_voxel_spawn_threshold"), &VoxelTerrain::get_noise_voxel_spawn_threshold);
	ClassDB::bind_method(D_METHOD("set_noise_voxel_spawn_threshold", "new_noise_voxel_spawn_threshold"), &VoxelTerrain::set_noise_voxel_spawn_threshold);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "noise_voxel_spawn_threshold"), "set_noise_voxel_spawn_threshold", "get_noise_voxel_spawn_threshold");
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
    create_noise_terrain_SVO();
}


void VoxelTerrain::_process(double delta)
{
    _process_geometry_updates();
}


///////////////////
// Public Functions


void VoxelTerrain::remove_voxel(Vector3 voxel_position)
{
    //UtilityFunctions::print("VoxelTerrain remove_voxel()");
    sparse_voxel_octree.insert_voxel(voxel_position.x, voxel_position.y, voxel_position.z, 0);
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

    Ref<FastNoiseLite> noise_generator;
    noise_generator.instantiate();
    noise_generator->set_noise_type(_noise_type);
    noise_generator->set_frequency(_noise_frequency);

    // NOTE: This breaks if you output too many godot debug statements!
    std::chrono::time_point<std::chrono::steady_clock> start_time = std::chrono::steady_clock::now();
    for (int x = 0; x < terrain_chunk_size; x++)
    {
        for (int y = 0; y < terrain_chunk_size; y++)
        {
            for (int z = 0; z < terrain_chunk_size; z++)
            {
                float noise_value = noise_generator->get_noise_3d(x, y, z);
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


// Nothing yet


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

        _generate_culling_voxel_mesh();

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


void VoxelTerrain::_generate_collider()
{
    // TODO: Use the collision node I generate on _ready() and procedurally generate the mesh myself
    // Can hopefully use the same algorithm as mesh generation and that may be way faster than trimesh generation done by godot.

    if (!_mesh_node) { return; }

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
    /*
     * NOTE ISSUE: Current implementation is crashing due to flood fill recursion, possible stack overflow.
     * It is fine with 5/6 calls, but a 6th call causes a crash.
     * Additionally, if I have all 6 calls active but change the terrain chunk size to 32 instead of 64, then it is also fine.
     */

    std::vector<VoxelUtility::VoxelGrid> extracted_voxel_grids;
    // Store voxels with their position and voxel value in a separate vector as the full grid size
    // isn't known until the flood fill is complete
    std::vector<std::vector<VoxelUtility::ExtractedVoxel>> extracted_voxels;
    std::vector<bool> visited_voxels(terrain_chunk_size*terrain_chunk_size*terrain_chunk_size, false);

    UtilityFunctions::print("\nFlood fill connected component start");

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
                    _flood_fill(x, y, z, visited_voxels, extracted_voxel_grids.back(),
                        extracted_voxels.back(), grid_min_corner, grid_max_corner);

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
    Vector3i& grid_min_corner, Vector3i& grid_max_corner)
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

        // Recursively scan in all orthogonal directions
        _flood_fill(x-1, y, z, visited_voxels, voxel_grid, extracted_voxels, grid_min_corner, grid_max_corner);
        _flood_fill(x+1, y, z, visited_voxels, voxel_grid, extracted_voxels, grid_min_corner, grid_max_corner);
        _flood_fill(x, y-1, z, visited_voxels, voxel_grid, extracted_voxels, grid_min_corner, grid_max_corner);
        _flood_fill(x, y+1, z, visited_voxels, voxel_grid, extracted_voxels, grid_min_corner, grid_max_corner);
        _flood_fill(x, y, z-1, visited_voxels, voxel_grid, extracted_voxels, grid_min_corner, grid_max_corner);
        //_flood_fill(x, y, z+1, visited_voxels, voxel_grid, extracted_voxels, grid_min_corner, grid_max_corner);
    }
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

    /*TEST*/
    VoxelUtility::VoxelGrid voxel_grid;
    voxel_grid.offset_in_parent_grid = Vector3i(0,5,0);
    bool is_grid_empty = true;
    for (int i = 0; i < 32; i++)
    {
        Vector3 voxel_position = Vector3(i,0,0) +  voxel_grid.offset_in_parent_grid;
        VoxelUtility::Voxel voxel = sparse_voxel_octree.get_voxel(voxel_position.x, voxel_position.y, voxel_position.z);
        voxel_grid.voxels.push_back(voxel);
        if (voxel != 0) { is_grid_empty = false; }
        remove_voxel(voxel_position);
    }
    if (!is_grid_empty)
    {
        voxel_grid.dimensions = Vector3i(32,1,1);
        VoxelObject* new_voxel_object = memnew(VoxelObject);
        new_voxel_object->initialise(voxel_grid.dimensions, voxel_grid.voxels);
        get_parent()->add_child(new_voxel_object);
        new_voxel_object->set_scale(get_scale());
        // Local position also works since I use the same parent.
        // NOTE: position should be offset by the lowest position voxel in the grid so
        // that it spawns in the correct position relative to the original grid (the terrain).
        new_voxel_object->set_global_position(get_global_position() + ( voxel_grid.offset_in_parent_grid * get_scale()));
        //new_voxel_object->set_freeze_enabled(true);
    }
    /*TEST END*/

    std::chrono::time_point<std::chrono::steady_clock> end_time = std::chrono::steady_clock::now();
    double elapsed_time = std::chrono::duration<double, std::milli>(end_time - start_time).count();
    UtilityFunctions::print("\nVoxelTerrain connected-component scan and body generation took ", elapsed_time, " ms");

    _mesh_needs_generated = true;
}
