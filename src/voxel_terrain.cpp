#include "voxel_terrain.h"
#include "voxel_utility.h"
#include <bitset>
#include <libmorton/morton.h>

using namespace godot;

void VoxelTerrain::_bind_methods() {

}

VoxelTerrain::VoxelTerrain() : sparse_voxel_octree(terrain_chunk_size) {

}

VoxelTerrain::~VoxelTerrain() {

}

void VoxelTerrain::_ready()
{
    morton_code_test();
}

void VoxelTerrain::_process(double delta)
{

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
