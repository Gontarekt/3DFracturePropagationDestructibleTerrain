#include "voxel_cube.h"
#include <cstdint>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/memory.hpp>
#include <godot_cpp/core/defs.hpp>
#include <memory>
#include <vector>
#include <chrono>
#include "voxel_utility.h"
#include <godot_cpp/classes/collision_shape3d.hpp>

using namespace godot;

void VoxelCube::_bind_methods() {
	ClassDB::bind_method(D_METHOD("get_size"), &VoxelCube::get_size);
	// Ideally I have a way to make the godot property always have a value of 1 or greater.
	ClassDB::bind_method(D_METHOD("set_size", "new_size"), &VoxelCube::set_size);
	ADD_PROPERTY(PropertyInfo(Variant::VECTOR3I, "size"), "set_size", "get_size");

	ClassDB::bind_method(D_METHOD("get_num_tests_to_run"), &VoxelCube::get_num_tests_to_run);
	ClassDB::bind_method(D_METHOD("set_num_tests_to_run", "new_num_tests_to_run"), &VoxelCube::set_num_tests_to_run);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "num_tests_to_run"), "set_num_tests_to_run", "get_num_tests_to_run");

	ClassDB::bind_method(D_METHOD("get_mesh_generation_type"), &VoxelCube::get_mesh_generation_type);
	ClassDB::bind_method(D_METHOD("set_mesh_generation_type", "new_mesh_generation_type"), &VoxelCube::set_mesh_generation_type);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "mesh_generation_type", PROPERTY_HINT_ENUM, "Triangle Strip Quad,Triangle List Quad,Culling Voxel Mesh"), "set_mesh_generation_type", "get_mesh_generation_type");
}

VoxelCube::VoxelCube() : size(Vector3i(1,1,1)), mesh_node(nullptr), num_voxels(1),
num_tests_to_run(1), mesh_needs_generated(true), mesh_generation_type(MeshGenerationType::culling_voxel_mesh)
{

}

VoxelCube::~VoxelCube()
{
	// Add your cleanup here.
}

void VoxelCube::_ready()
{
    // Generate the voxel array
    num_voxels = size.x*size.y*size.z;
    voxels = std::make_unique<std::vector<uint8_t>>(num_voxels, 1);
    if (voxels) {
        UtilityFunctions::print("Voxel array size: ", voxels->size());
    }

    mesh_node = dynamic_cast<MeshInstance3D*>(get_child(0));
}

void VoxelCube::_process(double delta)
{
    generate_mesh();
}

void VoxelCube::set_size(const Vector3i new_size)
{
    // Ensure no negative values
    size.x = std::max(new_size.x, 1);
    size.y = std::max(new_size.y, 1);
    size.z = std::max(new_size.z, 1);
    num_voxels = size.x*size.y*size.z;
    voxels = std::make_unique<std::vector<uint8_t>>(num_voxels, 1);
    mesh_needs_generated = true;
}

void VoxelCube::generate_triangle_strip_quad()
{
    if (!mesh_node) { return;}

    std::vector<Vector3> vertices_vector;
    PackedVector3Array vertices;
    vertices_vector.reserve(4);
    vertices_vector.push_back(Vector3(0,0,0));
    vertices_vector.push_back(Vector3(1,0,0));
    vertices_vector.push_back(Vector3(0,0,1));
    vertices_vector.push_back(Vector3(1,0,1));
    vertices.resize(vertices_vector.size());
    for (int i = 0; i < vertices_vector.size(); i ++)
    {
        vertices.set(i, vertices_vector[i]);
    }

    std::vector<Vector2> uvs_vector;
    PackedVector2Array uvs;
    uvs_vector.reserve(4);
    uvs_vector.push_back(Vector2(0,0));
    uvs_vector.push_back(Vector2(1,0));
    uvs_vector.push_back(Vector2(0,1));
    uvs_vector.push_back(Vector2(1,1));
    uvs.resize(uvs_vector.size());
    for (int i = 0; i < uvs_vector.size(); i ++)
    {
        uvs.set(i, uvs_vector[i]);
    }

    std::vector<Vector3> normals_vector;
    PackedVector3Array normals;
    normals_vector.reserve(4);
    normals_vector.push_back(Vector3(0,1,0));
    normals_vector.push_back(Vector3(0,1,0));
    normals_vector.push_back(Vector3(0,1,0));
    normals_vector.push_back(Vector3(0,1,0));
    normals.resize(normals_vector.size());
    for (int i = 0; i < normals_vector.size(); i ++)
    {
        normals.set(i, normals_vector[i]);
    }

    Array surface_array;
    surface_array.resize(Mesh::ArrayType::ARRAY_MAX);
    surface_array[(int)Mesh::ArrayType::ARRAY_VERTEX] = vertices;
    surface_array[(int)Mesh::ArrayType::ARRAY_TEX_UV] = uvs;
    surface_array[(int)Mesh::ArrayType::ARRAY_NORMAL] = normals;

    Ref<Mesh> mesh = mesh_node->get_mesh();
    if(ArrayMesh* array_mesh = Object::cast_to<ArrayMesh>(mesh.ptr()))
    {
        array_mesh->clear_surfaces();
        array_mesh->add_surface_from_arrays(Mesh::PrimitiveType::PRIMITIVE_TRIANGLE_STRIP, surface_array);
    }
}

void VoxelCube::generate_triangle_list_quad()
{
     if (!mesh_node) { return;}

     std::vector<Vector3> vertices_vector;
     PackedVector3Array vertices;
     vertices_vector.reserve(4);
     vertices_vector.push_back(Vector3(0,0,0));
     vertices_vector.push_back(Vector3(1,0,0));
     vertices_vector.push_back(Vector3(0,0,1));
     vertices_vector.push_back(Vector3(1,0,1));
     vertices.resize(vertices_vector.size());
     for (int i = 0; i < vertices_vector.size(); i ++)
     {
         vertices.set(i, vertices_vector[i]);
     }

     std::vector<Vector2> uvs_vector;
     PackedVector2Array uvs;
     uvs_vector.reserve(4);
     uvs_vector.push_back(Vector2(0,0));
     uvs_vector.push_back(Vector2(1,0));
     uvs_vector.push_back(Vector2(0,1));
     uvs_vector.push_back(Vector2(1,1));
     uvs.resize(uvs_vector.size());
     for (int i = 0; i < uvs_vector.size(); i ++)
     {
         uvs.set(i, uvs_vector[i]);
     }

     std::vector<Vector3> normals_vector;
     PackedVector3Array normals;
     normals_vector.reserve(4);
     normals_vector.push_back(Vector3(0,1,0));
     normals_vector.push_back(Vector3(0,1,0));
     normals_vector.push_back(Vector3(0,1,0));
     normals_vector.push_back(Vector3(0,1,0));
     normals.resize(normals_vector.size());
     for (int i = 0; i < normals_vector.size(); i ++)
     {
         normals.set(i, normals_vector[i]);
     }

     std::vector<int32_t> indices_vector;
     PackedInt32Array indices;
     indices_vector.reserve(6);
     indices_vector.push_back(0);
     indices_vector.push_back(1);
     indices_vector.push_back(2);
     indices_vector.push_back(1);
     indices_vector.push_back(3);
     indices_vector.push_back(2);
     indices.resize(indices_vector.size());
     for (int i = 0; i < indices_vector.size(); i ++)
     {
         indices.set(i, indices_vector[i]);
     }

     Array surface_array;
     surface_array.resize(Mesh::ArrayType::ARRAY_MAX);
     surface_array[(int)Mesh::ArrayType::ARRAY_VERTEX] = vertices;
     surface_array[(int)Mesh::ArrayType::ARRAY_TEX_UV] = uvs;
     surface_array[(int)Mesh::ArrayType::ARRAY_NORMAL] = normals;
     surface_array[(int)Mesh::ArrayType::ARRAY_INDEX] = indices; // Not needed with triangle strip

     //Ref<ArrayMesh> mesh = mesh_node->get_mesh(); // This works but is unsafe, crashes if mesh is not an ArrayMesh.
     Ref<Mesh> mesh = mesh_node->get_mesh();
     if(ArrayMesh* array_mesh = Object::cast_to<ArrayMesh>(mesh.ptr()))
     {
         array_mesh->clear_surfaces();
         array_mesh->add_surface_from_arrays(Mesh::PrimitiveType::PRIMITIVE_TRIANGLES, surface_array);
     }
}

void VoxelCube::set_num_tests_to_run(const int new_num_tests_to_run)
{
    num_tests_to_run = new_num_tests_to_run;
    num_tests_to_run = std::max(num_tests_to_run, 1);
}

void VoxelCube::generate_mesh()
{
    if (!mesh_node || !mesh_needs_generated) { return; }

    /// Procedurally generate voxel mesh from voxel grid

    timings.clear();
    for (int i = 0; i < num_tests_to_run; i++)
    {
        std::chrono::time_point<std::chrono::steady_clock> start_time = std::chrono::steady_clock::now();
        if (mesh_generation_type == MeshGenerationType::triangle_strip_quad){
            generate_triangle_strip_quad();
        }
        else if (mesh_generation_type == MeshGenerationType::triangle_list_quad){
            generate_triangle_list_quad();
        }
        else if (mesh_generation_type == MeshGenerationType::culling_voxel_mesh)
        {
            generate_voxel_mesh();
        }
        std::chrono::time_point<std::chrono::steady_clock> end_time = std::chrono::steady_clock::now();
        double elapsed_time = std::chrono::duration<double, std::milli>(end_time - start_time).count();
        timings.push_back(elapsed_time);
        // To prevent overflow of the debug output, don't output for every test if testing a lot of iterations.
        if (num_tests_to_run < 500)
        {
            UtilityFunctions::print("VoxelCube mesh generation took ", elapsed_time, " ms");
        }
    }
    double average_time = 0.0;
    for (auto time : timings)
    {
        average_time += time;
    }
    average_time /= timings.size();
    UtilityFunctions::print("VoxelCube mesh generation took an average of ", average_time, " ms");

    mesh_needs_generated = false;
    std::chrono::time_point<std::chrono::steady_clock> start_time = std::chrono::steady_clock::now();
    generate_collider();
    std::chrono::time_point<std::chrono::steady_clock> end_time = std::chrono::steady_clock::now();
    double elapsed_time = std::chrono::duration<double, std::milli>(end_time - start_time).count();
    UtilityFunctions::print("VoxelCube collider generation took ", elapsed_time, " ms");
}

void VoxelCube::generate_voxel_mesh()
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
    for (int x = 0; x < size.x; x++)
    {
        for (int y = 0; y < size.y; y++)
        {
            for (int z = 0; z < size.z; z++)
            {
                /// Ignore empty voxels

                int index = VoxelUtility::voxel_index_from_position(size.x, size.y, Vector3(x,y,z));
                if (voxels->at(index) == 0) { continue; }

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

                int index_adjacent_voxel = -1;
                if (y+1 < size.y) { index_adjacent_voxel = VoxelUtility::voxel_index_from_position(size.x, size.y, Vector3(x,y + 1,z)); };
                if (index_adjacent_voxel == -1 || voxels->at(index_adjacent_voxel) == 0)
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

                index_adjacent_voxel = -1;
                if (y-1 >= 0) { index_adjacent_voxel = VoxelUtility::voxel_index_from_position(size.x, size.y, Vector3(x,y - 1, z)); };
                if (index_adjacent_voxel == -1 || voxels->at(index_adjacent_voxel) == 0)
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

                index_adjacent_voxel = -1;
                if (x-1 >= 0) { index_adjacent_voxel = VoxelUtility::voxel_index_from_position(size.x, size.y, Vector3(x-1,y,z)); };
                if (index_adjacent_voxel == -1 || voxels->at(index_adjacent_voxel) == 0)
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

                index_adjacent_voxel = -1;
                if (x+1 < size.x) { index_adjacent_voxel = VoxelUtility::voxel_index_from_position(size.x, size.y, Vector3(x+1,y,z)); };
                if (index_adjacent_voxel == -1 || voxels->at(index_adjacent_voxel) == 0)
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

                index_adjacent_voxel = -1;
                if (z-1 >= 0) { index_adjacent_voxel = VoxelUtility::voxel_index_from_position(size.x, size.y, Vector3(x,y,z-1)); };
                if (index_adjacent_voxel == -1 || voxels->at(index_adjacent_voxel) == 0)
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

                index_adjacent_voxel = -1;
                if (z+1 < size.z) { index_adjacent_voxel = VoxelUtility::voxel_index_from_position(size.x, size.y, Vector3(x,y,z+1)); };
                if (index_adjacent_voxel == -1 || voxels->at(index_adjacent_voxel) == 0)
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

void VoxelCube::generate_collider()
{
    if (!mesh_node) { return; }
    // Note This seems to create a static body child, I need it only create a collider and under the same parent
    mesh_node->create_convex_collision();

    // Temp fix
    CollisionShape3D* collider_node = dynamic_cast<CollisionShape3D*>(mesh_node->get_child(0)->get_child(0));
    collider_node->reparent(this);
    mesh_node->get_child(0)->queue_free(); // Remove the static body
}
