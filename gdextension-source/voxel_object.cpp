#include "voxel_object.h"

using namespace godot;


void VoxelObject::_bind_methods() {
    BIND_VOXEL_NODE_PROPERTIES(VoxelObject);

	ClassDB::bind_method(D_METHOD("get_size"), &VoxelObject::get_size);
	ClassDB::bind_method(D_METHOD("set_size", "new_size"), &VoxelObject::set_size);
	ADD_PROPERTY(PropertyInfo(Variant::VECTOR3I, "size"), "set_size", "get_size");

	ClassDB::bind_method(D_METHOD("get_should_generate_solid_cube"), &VoxelObject::get_should_generate_solid_cube);
	ClassDB::bind_method(D_METHOD("set_should_generate_solid_cube", "new_should_generate_solid_cube"), &VoxelObject::set_should_generate_solid_cube);
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "should_generate_solid_cube"), "set_should_generate_solid_cube", "get_should_generate_solid_cube");
}


VoxelObject::VoxelObject()
{

}


VoxelObject::~VoxelObject()
{

}


void VoxelObject::initialise(Vector3i grid_size, VoxelUtility::VoxelVector voxel_grid)
{
    _size = grid_size;
    _voxels = std::make_unique<VoxelUtility::VoxelVector>(voxel_grid);

    _voxel_grid_initialised = true;
}


//////////////////
// Godot Functions


void VoxelObject::_ready()
{
    _initialise_nodes(this);

    // Generate the voxel array
    _num_voxels = _size.x*_size.y*_size.z;

    if (_should_generate_solid_cube) { _generate_solid_cube(); }
    else {  UtilityFunctions::print(""); } // Just to put the output onto a new line

    UtilityFunctions::print("VoxelObject VoxelVector NumBytes: ", sizeof(VoxelUtility::VoxelVector) + (sizeof(VoxelUtility::Voxel) * _voxels->capacity()));

    if (_voxels) {
        UtilityFunctions::print("Voxel array size: ", _voxels->size());
    }
}


void VoxelObject::_process(double delta)
{
    _process_geometry_updates();
}


void VoxelObject::remove_voxel(Vector3 voxel_position)
{
    UtilityFunctions::print("VoxelObject remove_voxel()");
    int voxel_index = VoxelUtility::voxel_index_from_position(_size.x, _size.y, voxel_position);
    _voxels->at(voxel_index) = 0;
}


////////////////////
// Getters & Setters


void VoxelObject::set_size(const Vector3i new_size)
{
    // Ensure no negative values
    _size.x = std::max(new_size.x, 1);
    _size.y = std::max(new_size.y, 1);
    _size.z = std::max(new_size.z, 1);
    _num_voxels = _size.x*_size.y*_size.z;
    _voxels = std::make_unique<VoxelUtility::VoxelVector>(_num_voxels, 1);
    _mesh_needs_generated = true;
}


//////////////////
// Mesh Generation


void VoxelObject::_generate_mesh()
{
    if (!_mesh_node || !_mesh_needs_generated) { return; }

    /// Procedurally generate voxel mesh from voxel grid

    UtilityFunctions::print("\nGenerating VoxelObject Mesh");
    _timings.clear();
    for (int i = 0; i < _num_mesh_generation_tests_to_run; i++)
    {
        std::chrono::time_point<std::chrono::steady_clock> start_time = std::chrono::steady_clock::now();
        _generate_voxel_mesh();
        std::chrono::time_point<std::chrono::steady_clock> end_time = std::chrono::steady_clock::now();
        double elapsed_time = std::chrono::duration<double, std::milli>(end_time - start_time).count();
        _timings.push_back(elapsed_time);
        // To prevent overflow of the debug output, don't output for every test if testing a lot of iterations.
        if (_num_mesh_generation_tests_to_run < 500)
        {
            UtilityFunctions::print("VoxelObject mesh generation took ", elapsed_time, " ms");
        }
    }
    double average_time = 0.0;
    for (auto time : _timings)
    {
        average_time += time;
    }
    average_time /= _timings.size();
    UtilityFunctions::print("VoxelObject mesh generation took an average of ", average_time, " ms");

    _mesh_needs_generated = false;
}


void VoxelObject::_generate_voxel_mesh()
{
    // NOTE: Godot has a clockwise rendering order

    if (!_mesh_node) { return;}

    //// VERTICES

    std::vector<Vector3> vertices_vector;
    PackedVector3Array vertices;
    std::vector<int32_t> indices_vector;
    PackedInt32Array indices;
    std::vector<Vector3> normals_vector;
    PackedVector3Array normals;
    // std::vector<Color> colours_vector;
    // PackedColorArray colours;
    // std::vector<Vector2> uvs_vector;
    // PackedVector2Array uvs;

    /// Generate vertices per voxel

    int vertex_offset = 0;
    for (int x = 0; x < _size.x; x++)
    {
        for (int y = 0; y < _size.y; y++)
        {
            for (int z = 0; z < _size.z; z++)
            {
                /// Ignore empty voxels

                int index = VoxelUtility::voxel_index_from_position(_size.x, _size.y, Vector3(x,y,z));
                if (_voxels->at(index) == 0) { continue; }

                //// Render voxel faces
                // Only render faces if the voxel adjacent to that face is empty, otherwise do not draw it (cull the face).

                /// Construct vertex positions (there are 8)
                // All faces drawn in Z shape, starting from top left.
                // x -+: left-right
                // y -+: bottom-top
                // z -+: front-back

                const Vector3 back_top_left = Vector3(-0.5 + x, 0.5 + y, 0.5 + z);
                const Vector3 back_top_right = Vector3(0.5 + x, 0.5 + y, 0.5 + z);
                const Vector3 back_bottom_left = Vector3(-0.5 + x, -0.5 + y, 0.5 + z);
                const Vector3 back_bottom_right = Vector3(0.5 + x, -0.5 + y, 0.5 + z);
                const Vector3 front_top_left = Vector3(-0.5 + x, 0.5 + y, -0.5 + z);
                const Vector3 front_top_right = Vector3(0.5 + x, 0.5 + y, -0.5 + z);
                const Vector3 front_bottom_left = Vector3(-0.5 + x, -0.5 + y, -0.5 + z);
                const Vector3 front_bottom_right = Vector3(0.5 + x, -0.5 + y, -0.5 + z);

                /// TOP FACE

                int index_adjacent_voxel = -1;
                if (y+1 < _size.y) { index_adjacent_voxel = VoxelUtility::voxel_index_from_position(_size.x, _size.y, Vector3(x,y + 1,z)); };
                if (index_adjacent_voxel == -1 || _voxels->at(index_adjacent_voxel) == 0)
                {
                    vertices_vector.push_back(front_top_left);
                    vertices_vector.push_back(front_top_right);
                    vertices_vector.push_back(back_top_left);
                    vertices_vector.push_back(back_top_right);

                    VoxelUtility::generate_indices_for_quad(indices_vector, vertex_offset);
                    VoxelUtility::generate_normals_for_quad(normals_vector, VoxelUtility::UP);

                    vertex_offset += 4;
                }

                /// BOTTOM FACE

                index_adjacent_voxel = -1;
                if (y-1 >= 0) { index_adjacent_voxel = VoxelUtility::voxel_index_from_position(_size.x, _size.y, Vector3(x,y - 1, z)); };
                if (index_adjacent_voxel == -1 || _voxels->at(index_adjacent_voxel) == 0)
                {
                    vertices_vector.push_back(back_bottom_left);
                    vertices_vector.push_back(back_bottom_right);
                    vertices_vector.push_back(front_bottom_left);
                    vertices_vector.push_back(front_bottom_right);

                    VoxelUtility::generate_indices_for_quad(indices_vector, vertex_offset);
                    VoxelUtility::generate_normals_for_quad(normals_vector, VoxelUtility::DOWN);

                    vertex_offset += 4;
                }

                /// LEFT FACE

                index_adjacent_voxel = -1;
                if (x-1 >= 0) { index_adjacent_voxel = VoxelUtility::voxel_index_from_position(_size.x, _size.y, Vector3(x-1,y,z)); };
                if (index_adjacent_voxel == -1 || _voxels->at(index_adjacent_voxel) == 0)
                {
                    vertices_vector.push_back(front_top_left);
                    vertices_vector.push_back(back_top_left);
                    vertices_vector.push_back(front_bottom_left);
                    vertices_vector.push_back(back_bottom_left);

                    VoxelUtility::generate_indices_for_quad(indices_vector, vertex_offset);
                    VoxelUtility::generate_normals_for_quad(normals_vector, VoxelUtility::LEFT);

                    vertex_offset += 4;
                }

                /// RIGHT FACE

                index_adjacent_voxel = -1;
                if (x+1 < _size.x) { index_adjacent_voxel = VoxelUtility::voxel_index_from_position(_size.x, _size.y, Vector3(x+1,y,z)); };
                if (index_adjacent_voxel == -1 || _voxels->at(index_adjacent_voxel) == 0)
                {
                    vertices_vector.push_back(back_top_right);
                    vertices_vector.push_back(front_top_right);
                    vertices_vector.push_back(back_bottom_right);
                    vertices_vector.push_back(front_bottom_right);

                    VoxelUtility::generate_indices_for_quad(indices_vector, vertex_offset);
                    VoxelUtility::generate_normals_for_quad(normals_vector, VoxelUtility::RIGHT);

                    vertex_offset += 4;
                }

                /// FRONT FACE

                index_adjacent_voxel = -1;
                if (z-1 >= 0) { index_adjacent_voxel = VoxelUtility::voxel_index_from_position(_size.x, _size.y, Vector3(x,y,z-1)); };
                if (index_adjacent_voxel == -1 || _voxels->at(index_adjacent_voxel) == 0)
                {
                    vertices_vector.push_back(front_top_right);
                    vertices_vector.push_back(front_top_left);
                    vertices_vector.push_back(front_bottom_right);
                    vertices_vector.push_back(front_bottom_left);

                    VoxelUtility::generate_indices_for_quad(indices_vector, vertex_offset);
                    VoxelUtility::generate_normals_for_quad(normals_vector, VoxelUtility::FORWARD);

                    vertex_offset += 4;
                }

                /// BACK FACE

                index_adjacent_voxel = -1;
                if (z+1 < _size.z) { index_adjacent_voxel = VoxelUtility::voxel_index_from_position(_size.x, _size.y, Vector3(x,y,z+1)); };
                if (index_adjacent_voxel == -1 || _voxels->at(index_adjacent_voxel) == 0)
                {
                    vertices_vector.push_back(back_top_left);
                    vertices_vector.push_back(back_top_right);
                    vertices_vector.push_back(back_bottom_left);
                    vertices_vector.push_back(back_bottom_right);

                    VoxelUtility::generate_indices_for_quad(indices_vector, vertex_offset);
                    VoxelUtility::generate_normals_for_quad(normals_vector, VoxelUtility::BACKWARD);

                    vertex_offset += 4;
                }
            }
        }
    }

    // Since the voxel grid is empty, remove the object.
    if (vertices_vector.size() <= 0)
    {
        queue_free();
        return;
    }

    /// Pack data into arrays

    vertices.resize(vertices_vector.size());
    for (int i = 0; i < vertices_vector.size(); i ++)
    {
        vertices.set(i, vertices_vector[i]);
    }
    indices.resize(indices_vector.size());
    for (int i = 0; i < indices_vector.size(); i ++)
    {
        indices.set(i, indices_vector[i]);
    }
    normals.resize(normals_vector.size());
    for (int i = 0; i < normals_vector.size(); i ++)
    {
        normals.set(i, normals_vector[i]);
    }
    // uvs.resize(uvs_vector.size());
    // for (int i = 0; i < uvs_vector.size(); i ++)
    // {
    //     uvs.set(i, uvs_vector[i]);
    // }

    /// Create surface array from generated mesh data

    Array surface_array;
    surface_array.resize(Mesh::ArrayType::ARRAY_MAX);
    surface_array[(int)Mesh::ArrayType::ARRAY_VERTEX] = vertices;
    surface_array[(int)Mesh::ArrayType::ARRAY_INDEX] = indices;
    surface_array[(int)Mesh::ArrayType::ARRAY_NORMAL] = normals;
    //surface_array[(int)Mesh::ArrayType::ARRAY_COLOR] = colours;
    //surface_array[(int)Mesh::ArrayType::ARRAY_TEX_UV] = uvs;

    /// Create mesh from Surface Array

    Ref<Mesh> mesh = _mesh_node->get_mesh();
    if(ArrayMesh* array_mesh = Object::cast_to<ArrayMesh>(mesh.ptr()))
    {
        array_mesh->clear_surfaces();
        array_mesh->add_surface_from_arrays(Mesh::PrimitiveType::PRIMITIVE_TRIANGLES, surface_array);
    }

    _collider_needs_generated = true;
}


//////////////////////////
// Other Private Functions


void VoxelObject::_generate_collider()
{
    if (!_mesh_node) { return; }

    std::chrono::time_point<std::chrono::steady_clock> start_time = std::chrono::steady_clock::now();

    // Note This seems to create a static body child, I need it only create a collider and under the same parent
    _mesh_node->create_convex_collision();
    //_mesh_node->create_trimesh_collision();

    // Temp fix
    if (_collider_node) {
        _collider_node->queue_free();
        _collider_node = nullptr;
    }
    _collider_node = dynamic_cast<CollisionShape3D*>(_mesh_node->get_child(0)->get_child(0));
    _collider_node->reparent(this);
    _mesh_node->get_child(0)->queue_free(); // Remove the static body

    std::chrono::time_point<std::chrono::steady_clock> end_time = std::chrono::steady_clock::now();
    double elapsed_time = std::chrono::duration<double, std::milli>(end_time - start_time).count();
    UtilityFunctions::print("VoxelCube collider generation took ", elapsed_time, " ms");
}


void VoxelObject::_generate_solid_cube()
{
    std::chrono::time_point<std::chrono::steady_clock> start_time = std::chrono::steady_clock::now();
    _voxels = std::make_unique<VoxelUtility::VoxelVector>(_num_voxels, 1);
    std::chrono::time_point<std::chrono::steady_clock> end_time = std::chrono::steady_clock::now();
    double elapsed_time = std::chrono::duration<double, std::milli>(end_time - start_time).count();
    UtilityFunctions::print("\nVoxelObject grid generation took ", elapsed_time, " ms");

    _voxel_grid_initialised = true;
}
