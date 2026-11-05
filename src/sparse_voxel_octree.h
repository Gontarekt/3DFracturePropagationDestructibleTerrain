/*
 * https://eisenwave.github.io/voxel-compression-docs/svo/construction.html
 * https://gist.github.com/eisenwave/c48bf988fc29d1c8bf0d4512d3916d22
 * Use this site to design the implemention of the SVO.
 *
 * Differences in my implementation:
 * - Compared to them, I use unsigned position, so no negative positions for voxels.
 * - The center of my octree is size/2
 * - I do not grow the octree, if position of a voxel is greater than the size of the octree then I will check if the terrain has an octree chunk for it,
 * otherwise it is ignored.
 * - They have a position normalisation step that I can skip due to not using negative positions.
 * It offsets all voxel positions so that there are no negative values,
 * e.g. -2,-2,-2 becomes 0,0,0 and 0,0,0 becomes 2,2,2 in a size 4 octree.
 * Notably it uses Space-filled curves and bit interleaving to index the voxel positions (3D) into the octree.
 *
 * Notes:
 * - They use Morton Order for interleaving the positions.
 *
 * Morton Codes 'Z-Order Curve' - https://en.wikipedia.org/wiki/Z-order_curve
 * - Z-Order Curve is a form of Space-filled curves
 * - Space-filled curves map Multi-dimensional coordinates into 1D.
 * - The benefits of space-filled curves in programming aren't just converted multi-dimensional to 1D, but also the locality of memory, which reduces cache
 * misses.
 * - Z-Order's particular benefit in voxels is that for each voxel, it has 2 mapped 2 of the adjacent voxels locally in memory, reducing cache misses.
 * - This flattens 3D positions into a 1D sequence using bit-interleaving.
 *
 * Rough Description:
 * - Root Node Represent the entire voxel chunk, so it will need a size. For convenience it should be a uniform x,y,z size.
 * - The root node will be subdivided into 8 regions if the voxels aren't all identical, which each represent 1/8 of that volume.
 * - Identical neighbouring voxels within an octant will be stored in one node. Otherwise if they differ, the node will subdivide further.
 * - If an entire node is empty, it will not be stored at all. Empty voxels will not waste memory unlike an array.
 * - Leaf nodes represent the smallest node in the octree. If a particular region is sparse then it could be as small as an individual voxel.
 *
 * Functionality:
 * - I need to convert a voxel array into an SVO after the terrain has been created.
 * To do that I need a function for inserting voxels individually (Can have a helper function which then uses this to insert voxel array)
 * - I need to be able to find a voxel in the SVO from a position (local to the chunk)
 * - I need to be able to convert voxel 3D position into a more efficient Morton position (1D) that the Octree will use.
 * - I need to be able to modify the SVO at runtime to remove voxels.
 * - I need to be able to recursively combine nodes with fully identical voxels after initial construction.
 * - Position from index?
 *
 * SO:
 * - IndexOf(Position)
 * - Insert(Index)
 *
 * - Is this position in octree occupied?
 * - Which children exist in this node?
 * - What dare is stored at this node?
 *
 * NOTE: I need to determine how I traverse the Octree using a Morton position
 *
 * SVO Pipeline:
 * - Construct Nodes from Voxel array
 * - Optimise SVO by removing nodes that have empty children and combining when all children have the same data.
 */

#include <cmath>
#include <cstdint>
#include <memory>
#include <array>
#include <vector>

class SparseVoxelOctree
{
    public:
        SparseVoxelOctree(size_t octree_axis_size) : _voxels_per_axis(octree_axis_size)
        {
            _calculate_max_depth();
        }
        ~SparseVoxelOctree() = default;

        struct Node {

        };

        struct Branch : public Node {
            // either stores a branch or leaf
            std::array<std::unique_ptr<Node>, 8> children;
        };

        struct Leaf : public Node {
            std::array<int8_t, 8> data{};
        };

        void generate_octree_from_voxel_array(std::vector<int8_t> voxel_array)
        {

        }

        void insert(int x, int y, int z, int8_t voxel)
        {
            int8_t index = _get_index_from_position(x, y, z);
            _find_or_create_node(index) = voxel;
        }

    private:

        void _calculate_max_depth()
        {
            _max_depth = std::log2(_voxels_per_axis);
        }

        // Assuming non-negative coords currently, I store voxels from 0,0,0 to size.x,size.y,size.z.
        // "the most significant bit of lower numbers is lower than the single bit of a greater power of two"
        // E.g. for 0111 which is 7, this will be less than 1000 which is 8.
        bool _is_voxel_inside_octree(int x, int y, int z)
        {
            return (x | y | z) >= _voxels_per_axis;
        }

        int64_t _get_index_from_position(int x, int y, int z)
        {
            // Convert 3D Position into Morton Code (Interweave the x,y,z into sequential 3 bit sequences which
            // provide the index at each level of the octree)
            // [0, 1, 2, 3] is -X. [4, 5, 6, 7] is +X.
            // [0, 1, 4, 5] is -Y. [2, 3, 6, 7] is +Y.
            // [0, 2, 4, 6] is -Z. [1, 3, 5, 7] is +Z.

            //isVoxelInsideSVO(x,y,z); // Don't think this is required for my implementation

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

        // Traverse the octree until it finds the value of the smallest node at the position interleaved in the index
        int8_t _find_node_value(int8_t octreeNodeIndex)
        {
            return 0;
        }

        // Traverses the Octree to reach the index, creating a leaf node at the exact position in maximum depth if
        // a leaf node doesn't already exist, if one does, it returns its value by reference.
        int8_t& _find_or_create_node(int8_t octreeNodeIndex)
        {
            // Should traverse using the index until it finds a leaf node or has fully read the morton code index
            // return the data value in that leaf node that the index points to.
            // Note: You need to determine the maximum depth of the octree to know how much of Morton code to read.

            // Recursively traverse octree until Morton code has been fully read or leaf node has been found.
            // TODO

        }

    private:
        int _voxels_per_axis; // Uniform Voxel Size in all axes. Must be a power of 2 for calculations to work!
        size_t _max_depth = 1; // Should calculate once after size has been set! Log base of 2 of the size will get maximum depth.
        // Recursivelly traverse the octree to find data (Leaf Nodes)
        std::unique_ptr<Branch> root = std::make_unique<Branch>();
};
