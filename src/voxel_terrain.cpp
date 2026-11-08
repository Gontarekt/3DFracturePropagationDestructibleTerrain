#include "voxel_terrain.h"
#include "godot_cpp/classes/fast_noise_lite.hpp"
#include "godot_cpp/classes/global_constants.hpp"
#include "voxel_utility.h"
#include <bitset>
#include <libmorton/morton.h>

using namespace godot;

void VoxelTerrain::_bind_methods() {
    ClassDB::bind_method(D_METHOD("get_num_tests_to_run"), &VoxelTerrain::get_num_tests_to_run);
	ClassDB::bind_method(D_METHOD("set_num_tests_to_run", "new_num_tests_to_run"), &VoxelTerrain::set_num_tests_to_run);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "num_tests_to_run"), "set_num_tests_to_run", "get_num_tests_to_run");

	ClassDB::bind_method(D_METHOD("remove_voxel", "voxel_position"), &VoxelTerrain::remove_voxel);
	ClassDB::bind_method(D_METHOD("create_noise_terrain_SVO"), &VoxelTerrain::create_noise_terrain_SVO);
	ClassDB::bind_method(D_METHOD("create_solid_cube_SVO"), &VoxelTerrain::create_solid_cube_SVO);

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

	ClassDB::bind_method(D_METHOD("get_mesh_needs_generated"), &VoxelTerrain::get_mesh_needs_generated);
	ClassDB::bind_method(D_METHOD("set_mesh_needs_generated", "new_mesh_needs_generated"), &VoxelTerrain::set_mesh_needs_generated);
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "mesh_needs_generated"), "set_mesh_needs_generated", "get_mesh_needs_generated");
}

VoxelTerrain::VoxelTerrain() : sparse_voxel_octree(terrain_chunk_size), mesh_node(nullptr),
num_tests_to_run(1), mesh_needs_generated(true), collider_node(nullptr),
noise_frequency(0.02f), noise_type(FastNoiseLite::NoiseType::TYPE_SIMPLEX), noise_voxel_spawn_threshold(0.2f) {

}

VoxelTerrain::~VoxelTerrain() {

}

void VoxelTerrain::_ready()
{
    morton_code_test();
    //create_solid_cube_SVO();
    create_noise_terrain_SVO();
    mesh_node = dynamic_cast<MeshInstance3D*>(get_child(0));
}

void VoxelTerrain::_process(double delta)
{
    generate_mesh();
}

void VoxelTerrain::morton_code_test()
{
    /*
     * Seems lib morton will place XYZ from LSB - MSB (so X is LSB and Z is MSB). Also the leftmost sequence of 3 bits is the starting point instead
     * of the 3 bits on the far right of the Morton code integer.
     */
    int64_t index1 = libmorton::morton3D_64_encode(0,0,0);
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

    UtilityFunctions::print("Morton Code 1 : ", std::bitset<64>(index1).to_string().c_str(), " int: ", index1);
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
}

void VoxelTerrain::create_noise_terrain_SVO()
{
    // Clear Previous Voxels
    sparse_voxel_octree.clear_voxels();

    Ref<FastNoiseLite> noise_generator;
    noise_generator.instantiate();
    noise_generator->set_noise_type(noise_type);
    noise_generator->set_frequency(noise_frequency);

    // NOTE: This breaks if you output too many godot debug statements!
    std::chrono::time_point<std::chrono::steady_clock> start_time = std::chrono::steady_clock::now();
    for (int x = 0; x < terrain_chunk_size; x++)
    {
        for (int y = 0; y < terrain_chunk_size; y++)
        {
            for (int z = 0; z < terrain_chunk_size; z++)
            {
                float noise_value = noise_generator->get_noise_3d(x, y, z);
                if (noise_value > noise_voxel_spawn_threshold)
                {
                    sparse_voxel_octree.insert_voxel(x,y,z,1);
                }
            }
        }
    }
    std::chrono::time_point<std::chrono::steady_clock> end_time = std::chrono::steady_clock::now();
    double elapsed_time = std::chrono::duration<double, std::milli>(end_time - start_time).count();
    UtilityFunctions::print("Terrain Noise generation took: ", elapsed_time, " ms");
}

void VoxelTerrain::generate_mesh()
{
    if (!mesh_node || !mesh_needs_generated) { return; }

    /// Procedurally generate voxel mesh from voxel grid

    timings.clear();
    for (int i = 0; i < num_tests_to_run; i++)
    {
        std::chrono::time_point<std::chrono::steady_clock> start_time = std::chrono::steady_clock::now();

        generate_culling_voxel_mesh();

        std::chrono::time_point<std::chrono::steady_clock> end_time = std::chrono::steady_clock::now();
        double elapsed_time = std::chrono::duration<double, std::milli>(end_time - start_time).count();
        timings.push_back(elapsed_time);
        // To prevent overflow of the debug output, don't output for every test if testing a lot of iterations.
        if (num_tests_to_run < 500)
        {
            UtilityFunctions::print("VoxelTerrain mesh generation took ", elapsed_time, " ms");
        }
    }
    double average_time = 0.0;
    for (auto time : timings)
    {
        average_time += time;
    }
    average_time /= timings.size();
    UtilityFunctions::print("VoxelTerrain mesh generation took an average of ", average_time, " ms");

    mesh_needs_generated = false;
    std::chrono::time_point<std::chrono::steady_clock> start_time = std::chrono::steady_clock::now();
    generate_collider();
    std::chrono::time_point<std::chrono::steady_clock> end_time = std::chrono::steady_clock::now();
    double elapsed_time = std::chrono::duration<double, std::milli>(end_time - start_time).count();
    UtilityFunctions::print("VoxelTerrain collider generation took ", elapsed_time, " ms");
}

/*
 * TODO: Make this take advantage of the octree so that it only loops through existing voxels instead of checking every voxel!
 * I can check for the existing voxels via traversal of the octree (Although maybe this is actually slower, this is something to test)
 */
void VoxelTerrain::generate_culling_voxel_mesh()
{
    if (!mesh_node) { return;}

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

    Ref<Mesh> mesh = mesh_node->get_mesh();
    if(ArrayMesh* array_mesh = Object::cast_to<ArrayMesh>(mesh.ptr()))
    {
        array_mesh->clear_surfaces();
        array_mesh->add_surface_from_arrays(Mesh::PrimitiveType::PRIMITIVE_TRIANGLES, surface_array);
    }
}

void VoxelTerrain::generate_collider()
{
    if (!mesh_node) { return; }
    // Note This seems to create a static body child, I need it only create a collider and under the same parent
    //mesh_node->create_convex_collision();
    mesh_node->create_trimesh_collision();

    // Temp fix
    if (collider_node) {
        collider_node->queue_free();
        collider_node = nullptr;
    }
    collider_node = dynamic_cast<CollisionShape3D*>(mesh_node->get_child(0)->get_child(0));
    collider_node->reparent(this);
    mesh_node->get_child(0)->queue_free(); // Remove the static body
}

void VoxelTerrain::set_num_tests_to_run(const int new_num_tests_to_run)
{
    num_tests_to_run = new_num_tests_to_run;
    num_tests_to_run = std::max(num_tests_to_run, 1);
}

void VoxelTerrain::remove_voxel(Vector3 voxel_position)
{
    UtilityFunctions::print("VoxelTerrain remove_voxel()");
    sparse_voxel_octree.insert_voxel(voxel_position.x, voxel_position.y, voxel_position.z, 0);
    mesh_needs_generated = true;
}
