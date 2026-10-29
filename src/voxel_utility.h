#include <godot_cpp/variant/Vector3.hpp>

namespace VoxelUtility
{
    // https://cplusplus.com/forum/general/137677/
    // https://stackoverflow.com/questions/7367770/how-to-flatten-or-index-3d-array-in-1d-array
    inline int voxel_index_from_position(int width, int height, int x, int y, int z)
    {
        return (int)(x + (y * width) + (z*width*height));
    }

    // https://stackoverflow.com/questions/7367770/how-to-flatten-or-index-3d-array-in-1d-array
    inline godot::Vector3 voxel_position_from_index(const int index, const int width, const int height)
    {
        int temp = index;
        int z = temp / (width * height);
        temp -= (z * width * height);
        int y = temp / width;
        int x = index % width;
        return godot::Vector3(x, y, z);
    }
}
