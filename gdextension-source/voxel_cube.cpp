#include "voxel_cube.h"

using namespace godot;


void VoxelCube::_bind_methods() {
    BIND_VOXEL_NODE_PROPERTIES(VoxelCube);

	ClassDB::bind_method(D_METHOD("get_size"), &VoxelCube::get_size);
	// Ideally I have a way to make the godot property always have a value of 1 or greater.
	ClassDB::bind_method(D_METHOD("set_size", "new_size"), &VoxelCube::set_size);
	ADD_PROPERTY(PropertyInfo(Variant::VECTOR3I, "size"), "set_size", "get_size");

	ClassDB::bind_method(D_METHOD("get_mesh_generation_type"), &VoxelCube::get_mesh_generation_type);
	ClassDB::bind_method(D_METHOD("set_mesh_generation_type", "new_mesh_generation_type"), &VoxelCube::set_mesh_generation_type);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "mesh_generation_type", PROPERTY_HINT_ENUM,
	    "Triangle Strip Quad,Triangle List Quad,Culling Voxel Mesh"), "set_mesh_generation_type", "get_mesh_generation_type");
}


VoxelCube::VoxelCube() :
_size(Vector3i(1,1,1)),
_num_voxels(1),
_mesh_generation_type(MeshGenerationType::culling_voxel_mesh)
{

}


VoxelCube::~VoxelCube()
{
	// Add your cleanup here.
}


//////////////////
// Godot Functions


void VoxelCube::_ready()
{
    _initialise_nodes(this);

    // Generate the voxel array
    _num_voxels = _size.x*_size.y*_size.z;

    std::chrono::time_point<std::chrono::steady_clock> start_time = std::chrono::steady_clock::now();
    _voxels = std::make_unique<VoxelUtility::VoxelVector>(_num_voxels, 1);
    std::chrono::time_point<std::chrono::steady_clock> end_time = std::chrono::steady_clock::now();
    double elapsed_time = std::chrono::duration<double, std::milli>(end_time - start_time).count();
    UtilityFunctions::print("\nVoxelCube grid generation took ", elapsed_time, " ms");
    UtilityFunctions::print("VoxelCube NumBytes: ", sizeof(VoxelUtility::VoxelVector) + (sizeof(VoxelUtility::Voxel) * _voxels->capacity()));

    if (_voxels) {
        UtilityFunctions::print("Voxel array size: ", _voxels->size());
    }
}


void VoxelCube::_process(double delta)
{
    _process_geometry_updates();
}


///////////////////
// Public Functions


void VoxelCube::remove_voxel(Vector3 voxel_position)
{
    UtilityFunctions::print("VoxelCube remove_voxel()");
    int voxel_index = VoxelUtility::voxel_index_from_position(_size.x, _size.y, voxel_position);
    _voxels->at(voxel_index) = 0;
    _mesh_needs_generated = true;
}


////////////////////
// Getters & Setters


void VoxelCube::set_size(const Vector3i new_size)
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


void VoxelCube::_generate_mesh()
{
    if (!_mesh_node || !_mesh_needs_generated) { return; }

    /// Procedurally generate voxel mesh from voxel grid

    UtilityFunctions::print("\nGenerating VoxelCube Mesh");
    _timings.clear();
    for (int i = 0; i < _num_mesh_generation_tests_to_run; i++)
    {
        std::chrono::time_point<std::chrono::steady_clock> start_time = std::chrono::steady_clock::now();
        if (_mesh_generation_type == MeshGenerationType::triangle_strip_quad){
            _generate_triangle_strip_quad();
        }
        else if (_mesh_generation_type == MeshGenerationType::triangle_list_quad){
            _generate_triangle_list_quad();
        }
        else if (_mesh_generation_type == MeshGenerationType::culling_voxel_mesh)
        {
            _generate_voxel_mesh();
        }
        std::chrono::time_point<std::chrono::steady_clock> end_time = std::chrono::steady_clock::now();
        double elapsed_time = std::chrono::duration<double, std::milli>(end_time - start_time).count();
        _timings.push_back(elapsed_time);
        // To prevent overflow of the debug output, don't output for every test if testing a lot of iterations.
        if (_num_mesh_generation_tests_to_run < 500)
        {
            UtilityFunctions::print("VoxelCube mesh generation took ", elapsed_time, " ms");
        }
    }
    double average_time = 0.0;
    for (auto time : _timings)
    {
        average_time += time;
    }
    average_time /= _timings.size();
    UtilityFunctions::print("VoxelCube mesh generation took an average of ", average_time, " ms");

    _mesh_needs_generated = false;
    _collider_needs_generated = true;
}


void VoxelCube::_generate_triangle_strip_quad()
{
    if (!_mesh_node) { return;}

    std::vector<Vector3> vertices_vector;
    PackedVector3Array vertices;
    vertices_vector.reserve(4);
    vertices_vector.push_back(Vector3(0,0,0));
    vertices_vector.push_back(Vector3(1,0,0));
    vertices_vector.push_back(Vector3(0,0,1));
    vertices_vector.push_back(Vector3(1,0,1));
    vertices.resize(vertices_vector.size());
    for (int i = 0; i < vertices_vector.size(); i ++)
    {
        vertices.set(i, vertices_vector[i]);
    }

    std::vector<Vector2> uvs_vector;
    PackedVector2Array uvs;
    uvs_vector.reserve(4);
    uvs_vector.push_back(Vector2(0,0));
    uvs_vector.push_back(Vector2(1,0));
    uvs_vector.push_back(Vector2(0,1));
    uvs_vector.push_back(Vector2(1,1));
    uvs.resize(uvs_vector.size());
    for (int i = 0; i < uvs_vector.size(); i ++)
    {
        uvs.set(i, uvs_vector[i]);
    }

    std::vector<Vector3> normals_vector;
    PackedVector3Array normals;
    normals_vector.reserve(4);
    normals_vector.push_back(Vector3(0,1,0));
    normals_vector.push_back(Vector3(0,1,0));
    normals_vector.push_back(Vector3(0,1,0));
    normals_vector.push_back(Vector3(0,1,0));
    normals.resize(normals_vector.size());
    for (int i = 0; i < normals_vector.size(); i ++)
    {
        normals.set(i, normals_vector[i]);
    }

    Array surface_array;
    surface_array.resize(Mesh::ArrayType::ARRAY_MAX);
    surface_array[(int)Mesh::ArrayType::ARRAY_VERTEX] = vertices;
    surface_array[(int)Mesh::ArrayType::ARRAY_TEX_UV] = uvs;
    surface_array[(int)Mesh::ArrayType::ARRAY_NORMAL] = normals;

    Ref<Mesh> mesh = _mesh_node->get_mesh();
    if(ArrayMesh* array_mesh = Object::cast_to<ArrayMesh>(mesh.ptr()))
    {
        array_mesh->clear_surfaces();
        array_mesh->add_surface_from_arrays(Mesh::PrimitiveType::PRIMITIVE_TRIANGLE_STRIP, surface_array);
    }
}


void VoxelCube::_generate_triangle_list_quad()
{
     if (!_mesh_node) { return;}

     std::vector<Vector3> vertices_vector;
     PackedVector3Array vertices;
     vertices_vector.reserve(4);
     vertices_vector.push_back(Vector3(0,0,0));
     vertices_vector.push_back(Vector3(1,0,0));
     vertices_vector.push_back(Vector3(0,0,1));
     vertices_vector.push_back(Vector3(1,0,1));
     vertices.resize(vertices_vector.size());
     for (int i = 0; i < vertices_vector.size(); i ++)
     {
         vertices.set(i, vertices_vector[i]);
     }

     std::vector<Vector2> uvs_vector;
     PackedVector2Array uvs;
     uvs_vector.reserve(4);
     uvs_vector.push_back(Vector2(0,0));
     uvs_vector.push_back(Vector2(1,0));
     uvs_vector.push_back(Vector2(0,1));
     uvs_vector.push_back(Vector2(1,1));
     uvs.resize(uvs_vector.size());
     for (int i = 0; i < uvs_vector.size(); i ++)
     {
         uvs.set(i, uvs_vector[i]);
     }

     std::vector<Vector3> normals_vector;
     PackedVector3Array normals;
     normals_vector.reserve(4);
     normals_vector.push_back(Vector3(0,1,0));
     normals_vector.push_back(Vector3(0,1,0));
     normals_vector.push_back(Vector3(0,1,0));
     normals_vector.push_back(Vector3(0,1,0));
     normals.resize(normals_vector.size());
     for (int i = 0; i < normals_vector.size(); i ++)
     {
         normals.set(i, normals_vector[i]);
     }

     std::vector<int32_t> indices_vector;
     PackedInt32Array indices;
     indices_vector.reserve(6);
     indices_vector.push_back(0);
     indices_vector.push_back(1);
     indices_vector.push_back(2);
     indices_vector.push_back(1);
     indices_vector.push_back(3);
     indices_vector.push_back(2);
     indices.resize(indices_vector.size());
     for (int i = 0; i < indices_vector.size(); i ++)
     {
         indices.set(i, indices_vector[i]);
     }

     Array surface_array;
     surface_array.resize(Mesh::ArrayType::ARRAY_MAX);
     surface_array[(int)Mesh::ArrayType::ARRAY_VERTEX] = vertices;
     surface_array[(int)Mesh::ArrayType::ARRAY_TEX_UV] = uvs;
     surface_array[(int)Mesh::ArrayType::ARRAY_NORMAL] = normals;
     surface_array[(int)Mesh::ArrayType::ARRAY_INDEX] = indices; // Not needed with triangle strip

     //Ref<ArrayMesh> mesh = mesh_node->get_mesh(); // This works but is unsafe, crashes if mesh is not an ArrayMesh.
     Ref<Mesh> mesh = _mesh_node->get_mesh();
     if(ArrayMesh* array_mesh = Object::cast_to<ArrayMesh>(mesh.ptr()))
     {
         array_mesh->clear_surfaces();
         array_mesh->add_surface_from_arrays(Mesh::PrimitiveType::PRIMITIVE_TRIANGLES, surface_array);
     }
}


void VoxelCube::_generate_voxel_mesh()
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
}


//////////////////////////
// Other Private Functions


void VoxelCube::_generate_collider()
{
    if (!_mesh_node) { return; }

    std::chrono::time_point<std::chrono::steady_clock> start_time = std::chrono::steady_clock::now();

    // Note This seems to create a static body child, I need it only create a collider and under the same parent
    //mesh_node->create_convex_collision();
    _mesh_node->create_trimesh_collision();

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
