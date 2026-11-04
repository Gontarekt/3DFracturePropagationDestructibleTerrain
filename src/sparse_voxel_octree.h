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
 * - I need to be able to find a voxel in the SVO from a position (local to the chunk)
 * - I need to be able to modify the SVO at runtime to remove voxels.
 *
 * - Is this position in octree occupied?
 * - Which children exist in this node?
 * - What dare is stored at this node?
 *
 * SVO Pipeline:
 * - Construct Nodes from Voxel array
 * - Optimise SVO by removing nodes that have empty children and combining when all children have the same data.
 */

#include <cstdint>
#include <memory>
#include <array>

class SparseVoxelOctree
{
    public:
        SparseVoxelOctree();
        ~SparseVoxelOctree();

        struct SVONode {

        };

        struct SVOBranch : public SVONode {
            // either stores a branch or leaf
            std::array<std::unique_ptr<SVONode>, 8> children;
        };

        struct SVOLeaf : public SVONode {
            std::array<int8_t, 8> data{};
        };

        void insert(size_t index, int8_t voxel)
        {

        }

    private:
        // Assuming non-negative coords currently, I store voxels from 0,0,0 to size.x,size.y,size.z.
        // "the most significant bit of lower numbers is lower than the single bit of a greater power of two"
        // E.g. for 0111 which is 7, this will be less than 1000 which is 8.
        bool isVoxelInsideSVO(int x, int y, int z)
        {
            return (x | y | z) >= size;
        }

        int8_t getOctreeIndexFromPosition(int x, int y, int z)
        {
            return 0;
        }

        int8_t* findOrCreateNode(int octreeNodeIndex)
        {
            return nullptr;
        }

    private:
        int size = 64; // Uniform Voxel Size in all axes. Must be a power of 2 for calculations to work!
        std::unique_ptr<SVOBranch> root = std::make_unique<SVOBranch>();
};
