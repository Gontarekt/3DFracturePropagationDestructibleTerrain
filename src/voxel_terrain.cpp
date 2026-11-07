#include "voxel_terrain.h"
#include "voxel_utility.h"
#include <bitset>
#include <libmorton/morton.h>

using namespace godot;

void VoxelTerrain::_bind_methods() {

}

VoxelTerrain::VoxelTerrain() : sparse_voxel_octree(64) {

}

VoxelTerrain::~VoxelTerrain() {

}

void VoxelTerrain::_ready()
{
    int64_t index1 = _get_index_from_position(0,0,0);
    // In a 2x2x2 grid I expect this to be right node (x axis) in a 1 depth octree
    // Expected Binary Output: 100
    int64_t index2 = _get_index_from_position(1,0,0);
    int64_t index3 = _get_index_from_position(0,1,0);
    int64_t index4 = _get_index_from_position(0,0,1);
    int64_t index5 = _get_index_from_position(1,1,1);
    int64_t index6 = _get_index_from_position(2,0,0);
    // In a 4x4x4 grid, since values are 0-3, with 3 being the furtherst point on that axis:
    // I expect this to be a depth of 2; From Root: Rightmost node on the x axis, From child of root: rightmost node on X. so 100,100
    // On the Z axis it would be rightmost node then leftmost. so 001, 000
    // Combined: 101, 100
    int64_t index7 = _get_index_from_position(3,0,2);
    // according to https://eisenwave.github.io/voxel-compression-docs/svo/construction.html
    // Expected binary: (0,1,1),(0,0,1)
    // Decimal: 25
    int64_t index8 = _get_index_from_position(6,8,9);
    int64_t index9 = _get_index_from_position(127,127,127);
    // int64_t index10 = ileave3(6, 8, 9);
    int64_t index10 = libmorton::morton3D_64_encode(6, 8, 9);

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
}

void VoxelTerrain::_process(double delta)
{

}
