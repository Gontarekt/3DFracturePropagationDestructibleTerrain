#include "sparse_voxel_octree.h"

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

int8_t SparseVoxelOctree::_find_node_value(int64_t octree_index)
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
            return leaf->data;
        }
        else {
            // Since we haven't fully read the Morton code yet, we should create new nodes
            // where there is a missing node.

            if (current_node->is_leaf)
            {
                Leaf* leaf = static_cast<Leaf*>(current_node);
                return leaf->data;
            }
            else
            {
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
    }
}

int8_t& SparseVoxelOctree::_find_or_create_node(int64_t octree_index, int8_t voxel)
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
    Branch* parent_node = nullptr;
    int parent_node_child_index = 0;
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
            return leaf->data;
        }
        else {
            // Since we haven't fully read the Morton code yet, we should create new nodes
            // where there is a missing node.

            // This section was likely previously optimised, or generated as a large voxel chunk
            // So we need to subdivide it if ihe voxel in the nodes region does not match the data.
            if (current_node->is_leaf)
            {
                Leaf* leaf = static_cast<Leaf*>(current_node);
                // If there is a leaf here and its datatype matches the voxel
                // then no need to make a new node / subdivide
                if (leaf->data == voxel)
                {
                    return leaf->data;
                }
                // As the leaf node is not the same data as the voxel, subdivision is required
                else {
                    int previous_data = leaf->data;
                    parent_node->children[parent_node_child_index] =
                        static_cast<std::unique_ptr<Node>>(std::make_unique<Branch>());
                    // Insert the previous leaf data into each new child node, then
                    // traverse to the node closest to the voxel position
                    Branch* new_branch = static_cast<Branch*>(parent_node->children[parent_node_child_index].get());
                    for (int child_index = 0; child_index < 8; child_index++)
                    {
                        std::unique_ptr<Leaf> new_child = std::make_unique<Leaf>();
                        new_child->data = previous_data;
                        new_branch->children[child_index] = std::move(new_child);
                    }
                    // Traverse to the new leaf node
                    parent_node = static_cast<Branch*>(current_node);
                    current_node = new_branch->children[node_index].get();
                    parent_node_child_index = node_index;
                }
            }
            else
            {
                Branch* branch = static_cast<Branch*>(current_node);
                Node* child_node = branch->children[node_index].get();
                if (child_node != nullptr)
                {
                    // Node found, traverse it on next pass.
                    parent_node = static_cast<Branch*>(current_node);
                    parent_node_child_index = node_index;
                    current_node = child_node;
                }
                else {
                    // Create new node.

                    // Convert branch to leaf if all leafs are identical
                    // NOTE: Proof of concept, this will only work on the final layer
                    if (i==MORTON_CODE_OFFSET_AT_MAXIMUM_DEPTH+3)
                    {
                        bool can_combine = true;
                        // Check parent and combine if data is uniform
                        for (int child_index = 0; child_index < 8; child_index++)
                        {
                            // If any of the other leafs don't match the new voxel data, then it can't combine
                            // TODO: May want to add trimming logic to remove nodes entirely if the voxel inserted
                            // is of value 0 (therefore removing voxel from the SVO)
                            if (child_index != node_index && (
                                !branch->children[child_index].get()
                                ||
                                static_cast<Leaf*>(branch->children[child_index].get())->data != voxel
                            )) {
                                    can_combine = false;
                                    break;
                            }
                        }
                        if (can_combine)
                        {
                            //godot::UtilityFunctions::print("Combining Node");
                            // We are on the final layer, so we need to access parent and change it

                            parent_node->children[parent_node_child_index] =
                                std::make_unique<Leaf>();
                            Leaf* new_leaf = static_cast<Leaf*>(parent_node->children[parent_node_child_index].get());
                            new_leaf->data = voxel;
                            return new_leaf->data;
                        }
                        else {
                            /// Create Leaf

                            std::unique_ptr<Leaf> new_child = std::make_unique<Leaf>();
                            branch->children[node_index] = std::move(new_child);
                            parent_node = static_cast<Branch*>(current_node);
                            parent_node_child_index = node_index;
                            current_node = branch->children[node_index].get();
                        }
                    }
                    else {
                        /// Create Branch

                        std::unique_ptr<Branch> new_child = std::make_unique<Branch>();
                        branch->children[node_index] = std::move(new_child);
                        parent_node = static_cast<Branch*>(current_node);
                        parent_node_child_index = node_index;
                        current_node = branch->children[node_index].get();
                    }
                }
            }
        }
    }
    // Prob put an assert here, this line should never be reached.
}
