#pragma once

#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/classes/static_body3d.hpp>
#include "sparse_voxel_octree.h"

namespace godot {

class VoxelTerrain : public StaticBody3D {
	GDCLASS(VoxelTerrain, StaticBody3D)

private:


protected:
	static void _bind_methods();

public:
	VoxelTerrain();
	~VoxelTerrain();

	void _ready() override;
	void _process(double delta) override;

	SparseVoxelOctree sparse_voxel_octree;

	//// TEMP
	int64_t _get_index_from_position(int x, int y, int z)
        {
            //isVoxelInsideSVO(x,y,z); // Don't think this is required for my implementation

            // Convert 3D Position into Morton Code (Interweave the x,y,z into sequential 3 bit sequences which
            // provide the index at each level of the octree)

            // https://graphics.stanford.edu/~seander/bithacks.html#InterleaveBMN
            // This uses the bit twiddling hacks from this site. Interweave the bits using magic numbers!
            constexpr size_t NUM_INPUTS = 3;
            constexpr uint64_t MASKS[] = {
                0x9249'2492'4924'9249,
                0x30C3'0C30'C30C'30C3,
                0xF00F'00F0'0F00'F00F,
                0x00FF'0000'FF00'00FF,
                0xFFFF'0000'0000'FFFF
            };

            // Interweave 2 0 bits after each bit value of the x, y & z position values
            // Mainly based off: https://eisenwave.github.io/voxel-compression-docs/svo/construction.html
            // TODO: I could probably rewrite this!
            uint64_t morton_code_x = x;
            for (int i = 4; i != 1; --i) {
                const auto shift = (NUM_INPUTS - 1) * (1 << i);
                morton_code_x |= morton_code_x << shift;
                morton_code_x &= MASKS[i];
            }
            uint64_t morton_code_y = y;
            for (int i = 4; i != 1; --i) {
                const auto shift = (NUM_INPUTS - 1) * (1 << i);
                morton_code_y |= morton_code_y << shift;
                morton_code_y &= MASKS[i];
            }
            uint64_t morton_code_z = z;
            for (int i = 4; i != 1; --i) {
                const auto shift = (NUM_INPUTS - 1) * (1 << i);
                morton_code_z |= morton_code_z << shift;
                morton_code_z &= MASKS[i];
            }
            // Combine the x,y & z morton values into a sequence of 3 bits (xyz, xyz, xyz... and so on)
            // Each bit will be a 0 to 1 value that represent left or right on that axis for each divison of the octree!
            return (morton_code_x << 2) | (morton_code_y << 1) | (morton_code_z);
        }

        uint64_t ileave_two0(uint32_t input)
        {
            constexpr size_t numInputs = 3;
            constexpr uint64_t masks[] = {
              0x9249'2492'4924'9249,
              0x30C3'0C30'C30C'30C3,
              0xF00F'00F0'0F00'F00F,
              0x00FF'0000'FF00'00FF,
              0xFFFF'0000'0000'FFFF
            };

            uint64_t n = input;
            for (int i = 4; i != 1; --i) {
                const auto shift = (numInputs - 1) * (1 << i);
                n |= n << shift;
                n &= masks[i];
            }

            return n;
        }

        uint64_t ileave3(uint32_t x, uint32_t y, uint32_t z)
        {
            return (ileave_two0(x) << 2) | (ileave_two0(y) << 1) | ileave_two0(z);
        }

    //// TEMP
};

} // namespace godot
