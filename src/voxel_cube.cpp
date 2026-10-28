#include "voxel_cube.h"
#include <cstdint>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/memory.hpp>
#include <godot_cpp/core/defs.hpp>
#include <memory>
#include <vector>

using namespace godot;

void VoxelCube::_bind_methods() {
	ClassDB::bind_method(D_METHOD("get_size"), &VoxelCube::get_size);
	// Ideally I have a way to make the godot property always have a value of 1 or greater.
	ClassDB::bind_method(D_METHOD("set_size", "new_size"), &VoxelCube::set_size);

	ADD_PROPERTY(PropertyInfo(Variant::VECTOR3I, "size"), "set_size", "get_size");
}

VoxelCube::VoxelCube() : size(Vector3i(1,1,1)), mesh_node(nullptr), num_voxels(1)
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
    if (mesh_node) {
        /// Procedurally generate voxel mesh from voxel grid

        std::vector<Vector3> vertices_vector;
        std::vector<Vector2> uvs_vector;
        std::vector<Vector3> normals_vector;
        std::vector<int32_t> indices_vector;
        PackedVector3Array vertices;
        PackedVector2Array uvs;
        PackedVector3Array normals;
        PackedInt32Array indices;

        vertices_vector.push_back(Vector3(0,0,0));
        vertices_vector.push_back(Vector3(0,0,1));
        vertices_vector.push_back(Vector3(1,0,0));
        vertices_vector.push_back(Vector3(1,0,1));
        vertices.resize(vertices_vector.size());
        for (int i = 0; i < vertices_vector.size(); i ++)
        {
            vertices.set(i, vertices_vector[i]);
        }

        uvs_vector.push_back(Vector2(0,0));
        uvs_vector.push_back(Vector2(1,0));
        uvs_vector.push_back(Vector2(0,1));
        uvs_vector.push_back(Vector2(1,1));
        uvs.resize(uvs_vector.size());
        for (int i = 0; i < uvs_vector.size(); i ++)
        {
            uvs.set(i, uvs_vector[i]);
        }

        normals_vector.push_back(Vector3(0,1,0));
        normals_vector.push_back(Vector3(0,1,0));
        normals_vector.push_back(Vector3(0,1,0));
        normals_vector.push_back(Vector3(0,1,0));
        normals.resize(normals_vector.size());
        for (int i = 0; i < normals_vector.size(); i ++)
        {
            normals.set(i, normals_vector[i]);
        }

        // indices_vector.push_back(0);
        // indices_vector.push_back(1);
        // indices_vector.push_back(2);
        // indices_vector.push_back(3);
        indices_vector.push_back(0);
        indices_vector.push_back(2);
        indices_vector.push_back(1);
        indices_vector.push_back(2);
        indices_vector.push_back(3);
        indices_vector.push_back(1);
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
        surface_array[(int)Mesh::ArrayType::ARRAY_INDEX] = indices;

        Ref<ArrayMesh> mesh = mesh_node->get_mesh();
        //mesh->add_surface_from_arrays(Mesh::PrimitiveType::PRIMITIVE_TRIANGLE_STRIP, surface_array);
        mesh->add_surface_from_arrays(Mesh::PrimitiveType::PRIMITIVE_TRIANGLES, surface_array);
    }
    if (voxels) {
        UtilityFunctions::print("Voxel array size: ", ((num_voxels) / sizeof(uint8_t)));
    }
}

void VoxelCube::_process(double delta)
{
    // if (voxels) {
    //     UtilityFunctions::print("Voxel array size: ", ((num_voxels) / sizeof(uint8_t)));
    // }
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
