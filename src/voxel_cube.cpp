#include "voxel_cube.h"
#include <godot_cpp/core/class_db.hpp>
#include <memory>

using namespace godot;

void VoxelCube::_bind_methods() {
	ClassDB::bind_method(D_METHOD("get_size"), &VoxelCube::get_size);
	// Ideally I have a way to make the godot property always have a value of 1 or greater.
	ClassDB::bind_method(D_METHOD("set_size", "newSize"), &VoxelCube::set_size);

	ADD_PROPERTY(PropertyInfo(Variant::VECTOR3I, "size"), "set_size", "get_size");
}

VoxelCube::VoxelCube() : size(Vector3i(0.f,0.f,0.f))
{

}

VoxelCube::~VoxelCube()
{
	// Add your cleanup here.
}

void VoxelCube::_ready()
{
    // Generate the voxel array
    voxels = std::make_unique<uint8_t[]>(size.x * size.y * size.z);
}

void VoxelCube::_process(double delta)
{
    if (voxels) {
        UtilityFunctions::print("Voxel array size: ", ((size.x * size.y * size.z) / sizeof(uint8_t)));
    }
}

void VoxelCube::set_size(const Vector3i newSize)
{
    // Ensure no negative values
    size.x = std::max(newSize.x, 1);
    size.y = std::max(newSize.y, 1);
    size.z = std::max(newSize.z, 1);
    voxels = std::make_unique<uint8_t[]>(size.x * size.y * size.z);
}
