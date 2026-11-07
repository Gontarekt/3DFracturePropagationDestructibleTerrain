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
 */

#include <cmath>
#include <cstdint>
#include <memory>
#include <array>
#include <vector>
#include <libmorton/morton.h>
#include <godot_cpp/classes/node3d.hpp> // For debug prints!

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

        // Make sure the byte size and alignment of the leaf matches the branch to
        // mitigate cache misses, might not need to make it the exact same size but just close.
        // TODO: Test if this actually is beneficial, also whether the memory cost outweighs the performance boost if any.
        struct Leaf : public Node {
            // Reduces cache misses by using an array at the second-to-final depth
            std::array<int32_t, 8> data{};
        };

        void generate_octree_from_voxel_array(std::vector<int8_t> voxel_array)
        {
            /// TODO
        }

        void insert_voxel(uint_fast32_t x, uint_fast32_t y, uint_fast32_t z, int32_t voxel)
        {
             godot::UtilityFunctions::print("\nSVO.insert_voxel():");
            int64_t octree_index = _get_index_from_position(x, y, z);
            int32_t& node_value = _find_or_create_node(octree_index);
             godot::UtilityFunctions::print("Node value in SVO at X: ", x, " Y: ", y, " Z: ", z, " was: ", node_value);
            node_value = voxel;
            godot::UtilityFunctions::print("Node value in SVO is now: ", node_value);
        }

        uint32_t get_voxel(uint_fast32_t x, uint_fast32_t y, uint_fast32_t z)
        {
            godot::UtilityFunctions::print("\nSVO.get_voxel():");
            int64_t octree_index = _get_index_from_position(x, y, z);
            uint32_t node_value = _find_node_value(octree_index);
            godot::UtilityFunctions::print("Node value found in SVO at : X: ", x, " Y: ", y, " Z: ", z, " is: ", node_value);
            return node_value;
        }

    private:

        void _calculate_max_depth()
        {
            _max_depth = std::log2(_voxels_per_axis);
        }

        // Assuming non-negative coords currently, I store voxels from 0,0,0 to size.x,size.y,size.z.
        // "the most significant bit of lower numbers is lower than the single bit of a greater power of two"
        // E.g. for 0111 which is 7, this will be less than 1000 which is 8.
        bool _is_voxel_inside_octree(uint_fast32_t x, uint_fast32_t y, uint_fast32_t z)
        {
            return (x | y | z) >= _voxels_per_axis;
        }

        int64_t _get_index_from_position(uint_fast32_t x, uint_fast32_t y, uint_fast32_t z)
        {
            /*
             * 0 bit represents (-) in that axis and 1 bit represent (+) in that axis (E.g. -x to x, or left to right).
             */
            return libmorton::morton3D_64_encode(x,y,z);
        }

        // Traverse the octree until it finds the value of the smallest node at the position interleaved in the index
        uint32_t _find_node_value(int64_t octree_index)
        {
            /*
             * If child of branch is nullptr then no voxels stored,
             * otherwise if leaf, check if value of data is 0 which then also means no voxel stored.
             */

            Node* current_node = root.get();
            for (int i = 3 * (_max_depth-1); i >= 0; i-=3)
            {
                uint_fast8_t node_index = (octree_index >> i) & 0b111;

                godot::UtilityFunctions::print(
                    "Morton digit: ", node_index,
                    " Bit position: ", i
                );

                // NOTE: Since we know what type a node is going to be at any given point,
                // use static_cast to avoid the overhead of dynamic cast!
                if (i == 0)
                {
                    godot::UtilityFunctions::print("current node is a leaf, returning data at index");

                    // If at the end of the morton code, then you can gaurantee it is a leaf node.
                    Leaf* leaf = static_cast<Leaf*>(current_node);
                    return leaf->data[node_index];
                }
                else {
                    // Since we haven't fully read the Morton code yet, we should create new nodes
                    // where there is a missing node.

                    Branch* branch = static_cast<Branch*>(current_node);
                    Node* child_node = branch->children[node_index].get();
                    if (child_node != nullptr)
                    {
                        // Node found, traverse it on next pass.
                        current_node = child_node;

                        godot::UtilityFunctions::print("Child is not nullptr, moving to that node next");
                    }
                    else {
                        godot::UtilityFunctions::print("Child is nullptr, no voxel at the position, return 0");

                        // as the branch is nullptr, this means no voxel data is stored at the position
                        return 0;
                    }
                }
            }
        }

        // Traverses the Octree to reach the index, creating a leaf node at the exact position in maximum depth if
        // a leaf node doesn't already exist. Returns the value of the leaf node by reference.
        int32_t& _find_or_create_node(int64_t octree_index)
        {
            // Should traverse using the index until it finds a leaf node or has fully read the morton code index
            // return the data value in the leaf node that the index points to.
            // Note: You need to determine the maximum depth of the octree to know how much of Morton code to read.

            /*
             * LibMorton places X as the LSB (right-most bit) and
             * also places the deepest node index in the LSB part of the Morton Code sequence.
             * So I need to use the depth to get the left-most point of the morton code.
             * The loop reads 3 bits at a time which represent the x,y,z of the node using only 1 bit.
             */
            Node* current_node = root.get();
            for (int i = 3 * (_max_depth-1); i >= 0; i-=3)
            {
                // This is simply a 0-7 value to get the index of the child at the current node
                uint_fast8_t node_index = (octree_index >> i) & 0b111;

                godot::UtilityFunctions::print(
                    "Morton digit: ", node_index,
                    " Bit position: ", i
                );

                // NOTE: Since we know what type a node is going to be at any given point,
                // use static_cast to avoid the overhead of dynamic cast!
                if (i == 0)
                {
                    godot::UtilityFunctions::print("current node is a leaf, returning data at index");

                    // If at the end of the morton code, then you can gaurantee it is a leaf node.
                    Leaf* leaf = static_cast<Leaf*>(current_node);
                    return leaf->data[node_index];
                }
                else {
                    // Since we haven't fully read the Morton code yet, we should create new nodes
                    // where there is a missing node.

                    Branch* branch = static_cast<Branch*>(current_node);
                    Node* child_node = branch->children[node_index].get();
                    if (child_node != nullptr)
                    {
                        // Node found, traverse it on next pass.
                        current_node = child_node;

                        godot::UtilityFunctions::print("Child is not nullptr, moving to that node next");
                    }
                    else {
                        // Create new node.

                        godot::UtilityFunctions::print("Child is nullptr, creating new node");

                        // If this is the pen-ultimate depth, create a leaf node, otherwise branch.
                        // (Remember the final depth will be represented by the data array in the leaf!)
                        std::unique_ptr<Node> new_child = i == 3
                            ? static_cast<std::unique_ptr<Node>>(std::make_unique<Leaf>())
                            : static_cast<std::unique_ptr<Node>>(std::make_unique<Branch>());
                        current_node = new_child.get();
                        branch->children[node_index] = std::move(new_child);
                    }
                }
            }
            // Prob put an assert here, this line should never be reached.
        }

    private:
        int _voxels_per_axis; // Uniform Voxel Size in all axes. Must be a power of 2 for calculations to work!
        size_t _max_depth = 1; // Should calculate once after size has been set! Log base of 2 of the size will get maximum depth.
        // Recursivelly traverse the octree to find data (Leaf Nodes)
        std::unique_ptr<Branch> root = std::make_unique<Branch>();
};
