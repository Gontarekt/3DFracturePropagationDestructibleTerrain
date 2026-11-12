/*
 * https://eisenwave.github.io/voxel-compression-docs/svo/construction.html
 * https://gist.github.com/eisenwave/c48bf988fc29d1c8bf0d4512d3916d22
 * Use this site to design the implemention of the SVO.
 *
 * Morton Codes: https://github.com/Forceflow/libmorton
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
 *
 * Limitations of implementation:
 * - This won't handle very small grid sizes that don't have a minimum depth of 2
 * The minimum this octree can represent is root->leaf->leafValue.
 * So that would make the minimum voxel grid size 4x4x4. However an octree would be a waste at those sizes anyway, it is expected to be used
 * for sizes of 16 and up.
 */

#pragma once
#include <cstdint>
#include <memory>
#include <array>
#include <libmorton/morton.h>
#include <godot_cpp/classes/node3d.hpp> // For debug prints!
#include "voxel_utility.h"

class SparseVoxelOctree
{
    public:
        SparseVoxelOctree(size_t octree_axis_size) : _voxels_per_axis(octree_axis_size)
        {
            _calculate_max_depth();
        }
        ~SparseVoxelOctree() = default;

        struct Node {
            //virtual ~Node() = default; // Required for it to be polymorphic
            bool is_leaf = false;
        };
        struct Branch : public Node {
            // either stores a branch or leaf
            std::array<std::unique_ptr<Node>, 8> children;
        };
        struct Leaf : public Node {
            Leaf(){is_leaf=true;}
            // Reduces memory usage by storing 8 voxels per leaf instead of creating an additional layer
            std::array<VoxelUtility::Voxel, 8> data{};
        };

        void insert_voxel(uint_fast32_t x, uint_fast32_t y, uint_fast32_t z, VoxelUtility::Voxel voxel);
        VoxelUtility::Voxel get_voxel(uint_fast32_t x, uint_fast32_t y, uint_fast32_t z);
        void clear_voxels();
        size_t get_total_bytes_in_memory() { return _get_num_bytes_in_memory_of_node(root.get()); }

    private:

        size_t _get_num_bytes_in_memory_of_node(const Node* node);
        void _calculate_max_depth();
        // Assuming non-negative coords currently, I store voxels from 0,0,0 to size.x,size.y,size.z.
        // "the most significant bit of lower numbers is lower than the single bit of a greater power of two"
        // E.g. for 0111 which is 7, this will be less than 1000 which is 8.
        bool _is_voxel_inside_octree(uint_fast32_t x, uint_fast32_t y, uint_fast32_t z);
        int64_t _get_index_from_position(uint_fast32_t x, uint_fast32_t y, uint_fast32_t z);

        // Traverse the octree until it finds the value of the smallest node at the position interleaved in the index
        VoxelUtility::Voxel _find_node_value(int64_t octree_index);
        // Traverses the Octree to reach the index, creating a leaf node at the exact position in maximum depth if
        // a leaf node doesn't already exist. Returns the value of the leaf node by reference.
        VoxelUtility::Voxel& _find_or_create_node(int64_t octree_index);

    private:
        // Uniform Voxel Size in all axes. Must be a power of 2 for calculations to work!
        int _voxels_per_axis;
        // Should calculate once after size has been set! Log base of 2 of the size will get maximum depth.
        size_t _max_depth = 1;
        // Recursivelly traverse the octree to find data (Leaf Nodes)
        std::unique_ptr<Branch> root = std::make_unique<Branch>();
        static constexpr int MORTON_CODE_OFFSET_AT_MAXIMUM_DEPTH = 0;
};
