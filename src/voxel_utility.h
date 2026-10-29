#include <cstdint>
#include <godot_cpp/variant/Vector3.hpp>
#include <godot_cpp/variant/Vector3i.hpp>
#include <vector>

namespace VoxelUtility
{
    inline const godot::Vector3i UP = { 0, 1, 0 };
    inline const godot::Vector3i DOWN = { 0, -1, 0 };
    inline const godot::Vector3i LEFT = {-1, 0, 0 };
    inline const godot::Vector3i RIGHT = { 1, 0, 0 };
    inline const godot::Vector3i FORWARD = { 0, 0, -1 };
    inline const godot::Vector3i BACKWARD = { 0, 0, 1 };

    // https://cplusplus.com/forum/general/137677/
    // https://stackoverflow.com/questions/7367770/how-to-flatten-or-index-3d-array-in-1d-array
    inline int voxel_index_from_position(int width, int height, godot::Vector3 position)
    {
        return (int)(position.x + (position.y * width) + (position.z*width*height));
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

    inline void generate_indices_for_quad(std::vector<int32_t>& indices, const int vertex_offset)
    {
        indices.push_back(0 + vertex_offset);
        indices.push_back(1 + vertex_offset);
        indices.push_back(2 + vertex_offset);
        indices.push_back(1 + vertex_offset);
        indices.push_back(3 + vertex_offset);
        indices.push_back(2 + vertex_offset);
    }

    inline void generate_normals_for_quad(std::vector<godot::Vector3>& normals, const godot::Vector3 normal)
    {
        normals.push_back(normal);
        normals.push_back(normal);
        normals.push_back(normal);
        normals.push_back(normal);
    }
}
