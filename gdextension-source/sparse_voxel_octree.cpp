#include "sparse_voxel_octree.h"


//////////////////////
// PUBLIC FUNCTIONS


void SparseVoxelOctree::insert_voxel(uint_fast32_t x, uint_fast32_t y, uint_fast32_t z, VoxelUtility::Voxel voxel)
{
    int64_t octree_index = _get_index_from_position(x, y, z);
    VoxelUtility::Voxel& node_value = _find_or_create_node(octree_index);
    node_value = voxel;
}


VoxelUtility::Voxel SparseVoxelOctree::get_voxel(uint_fast32_t x, uint_fast32_t y, uint_fast32_t z)
{
    int64_t octree_index = _get_index_from_position(x, y, z);
    VoxelUtility::Voxel node_value = _find_node_value(octree_index);

    return node_value;
}


void SparseVoxelOctree::clear_voxels()
{
    for (auto& child : root->children)
    {
        child = nullptr;
    }
}


size_t SparseVoxelOctree::_get_num_bytes_in_memory_of_node(const Node* node)
{
    if (node == nullptr) { return 0; }

    size_t total = 0;
    if (!node->is_leaf)
    {
        const Branch* branch = static_cast<const Branch*>(node);
        total += sizeof(*branch);
        for (const auto& child : branch->children)
        {
            total += _get_num_bytes_in_memory_of_node(child.get());
        }
    }
    else
    {
        const Leaf* leaf = static_cast<const Leaf*>(node);
        total += sizeof(*leaf);
    }
    return total;
}


//////////////////////
// PRIVATE FUNCTIONS


void SparseVoxelOctree::_calculate_max_depth()
{
    _max_depth = std::log2(_voxels_per_axis);
}


bool SparseVoxelOctree::_is_voxel_inside_octree(uint_fast32_t x, uint_fast32_t y, uint_fast32_t z)
{
    return (x | y | z) >= _voxels_per_axis;
}


int64_t SparseVoxelOctree::_get_index_from_position(uint_fast32_t x, uint_fast32_t y, uint_fast32_t z)
{
    // 0 bit represents (-) in that axis and 1 bit represent (+) in that axis (E.g. -x to x, or left to right).
    return libmorton::morton3D_64_encode(x,y,z);
}


//////////////////////
// FIND NODE FUNCTIONS


VoxelUtility::Voxel SparseVoxelOctree::_find_node_value(int64_t octree_index)
{
    /*
     * If child of branch is nullptr then no voxels stored,
     * otherwise if leaf, check if value of data is 0 which then also means no voxel stored.
     */

    Node* current_node = root.get();
    for (int i = 3 * (_max_depth-1); i >= MORTON_CODE_OFFSET_AT_MAXIMUM_DEPTH; i-=3)
    {
        uint_fast8_t node_index = (octree_index >> i) & 0b111;

        // NOTE: Since we know what type a node is going to be at any given point,
        // use static_cast to avoid the overhead of dynamic cast!
        if (i == MORTON_CODE_OFFSET_AT_MAXIMUM_DEPTH)
        {
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
            }
            else {
                // as the branch is nullptr, this means no voxel data is stored at the position
                return 0;
            }
        }
    }
    CRASH_NOW_MSG("Somehow _find_node_value() did not ever reach a leaf node, this should not happen");
    return -1;
}


VoxelUtility::Voxel& SparseVoxelOctree::_find_or_create_node(int64_t octree_index)
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
    for (int i = 3 * (_max_depth-1); i >= MORTON_CODE_OFFSET_AT_MAXIMUM_DEPTH; i-=3)
    {
        // This is simply a 0-7 value to get the index of the child at the current node
        uint_fast8_t node_index = (octree_index >> i) & 0b111;

        // NOTE: Since we know what type a node is going to be at any given point,
        // use static_cast to avoid the overhead of dynamic cast!
        if (i == MORTON_CODE_OFFSET_AT_MAXIMUM_DEPTH)
        {
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
            }
            else {
                // Create new node.

                // If this is the pen-ultimate depth, create a leaf node, otherwise branch.
                // (Remember the final depth will be represented by the data array in the leaf!)
                std::unique_ptr<Node> new_child = i == MORTON_CODE_OFFSET_AT_MAXIMUM_DEPTH+3
                    ? static_cast<std::unique_ptr<Node>>(std::make_unique<Leaf>())
                    : static_cast<std::unique_ptr<Node>>(std::make_unique<Branch>());
                current_node = new_child.get();
                branch->children[node_index] = std::move(new_child);
            }
        }
    }
    CRASH_NOW_MSG("Somehow _find_or_create_node() did not ever reach a leaf node, this should not happen");
    std::abort();
}
