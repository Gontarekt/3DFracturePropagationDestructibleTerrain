#include "voxel_cube.h"
#include <cstdint>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/memory.hpp>
#include <godot_cpp/core/defs.hpp>
#include <memory>
#include <vector>
#include <chrono>

using namespace godot;

void VoxelCube::_bind_methods() {
	ClassDB::bind_method(D_METHOD("get_size"), &VoxelCube::get_size);
	// Ideally I have a way to make the godot property always have a value of 1 or greater.
	ClassDB::bind_method(D_METHOD("set_size", "new_size"), &VoxelCube::set_size);
	ADD_PROPERTY(PropertyInfo(Variant::VECTOR3I, "size"), "set_size", "get_size");

	ClassDB::bind_method(D_METHOD("get_num_tests_to_run"), &VoxelCube::get_num_tests_to_run);
	// Ideally I have a way to make the godot property always have a value of 1 or greater.
	ClassDB::bind_method(D_METHOD("set_num_tests_to_run", "new_num_tests_to_run"), &VoxelCube::set_num_tests_to_run);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "num_tests_to_run"), "set_num_tests_to_run", "get_num_tests_to_run");

	ClassDB::bind_method(D_METHOD("get_use_triangle_strip_topology"), &VoxelCube::get_use_triangle_strip_topology);
	// Ideally I have a way to make the godot property always have a value of 1 or greater.
	ClassDB::bind_method(D_METHOD("set_use_triangle_strip_topology", "new_use_triangle_strip_topology"), &VoxelCube::set_use_triangle_strip_topology);
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "use_triangle_strip_topology"), "set_use_triangle_strip_topology", "get_use_triangle_strip_topology");
}

VoxelCube::VoxelCube() : size(Vector3i(1,1,1)), mesh_node(nullptr), num_voxels(1),
num_tests_to_run(1), mesh_needs_generated(true), use_triangle_strip_topology(true)
{

}

VoxelCube::~VoxelCube()
{
	// Add your cleanup here.
}

void VoxelCube::_ready()
{
    // Generate the voxel array
    voxels = std::make_unique<uint8_t[]>(num_voxels);
    mesh_node = dynamic_cast<MeshInstance3D*>(get_child(0));
    //generate_mesh();
    if (voxels) {
        UtilityFunctions::print("Voxel array size: ", ((num_voxels) / sizeof(uint8_t)));
    }
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
    voxels = std::make_unique<uint8_t[]>(num_voxels);
}

void VoxelCube::generate_triangle_strip_face()
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

void VoxelCube::generate_triangle_list_face()
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
        if (use_triangle_strip_topology){
            generate_triangle_strip_face();
        }
        else {
            generate_triangle_list_face();
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
}
