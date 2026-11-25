/*
 * Physically simulated Voxel Objects should use or inherit this class.
 * At minimumimum for a Voxel Object to be generated it will need:
 * - A vector of voxels
 * - Dimensions of the voxel grid (So it knows how to read the vector)
 * - The position of the voxel object in world-space (Lowest Corner).
 */


#include "godot_cpp/classes/rigid_body3d.hpp"
#include "voxel_node.h"
#include "voxel_utility.h"


namespace godot {
class VoxelObject : public RigidBody3D, public VoxelNode {
	GDCLASS(VoxelObject, RigidBody3D)

private:
	std::unique_ptr<VoxelUtility::VoxelVector> _voxels;
	Vector3i _size = Vector3i(1,1,1);
	int _num_voxels = 1; // Computed from size
	bool _should_generate_solid_cube = false;

protected:
	static void _bind_methods();

public:
	VoxelObject();
	~VoxelObject();

	void initialise(Vector3i grid_size, VoxelUtility::VoxelVector voxel_grid);

	// Godot Functions

	void _ready() override;
	void _process(double delta) override;

	// Public Functions

	void remove_voxel(Vector3i voxel_position) override;
	VoxelUtility::Voxel get_voxel(Vector3i voxel_position) override;

	// Getters & Setters

	void set_size(const Vector3i new_size);
	Vector3i get_size() const { return _size; }
	void set_should_generate_solid_cube(const bool new_should_generate_solid_cube) { _should_generate_solid_cube = new_should_generate_solid_cube; }
	bool get_should_generate_solid_cube() const { return _should_generate_solid_cube; }

private:
    // Mesh Generation
    // Example Functions for helping understanding of godots rendering + procedural mesh generation

	void _generate_mesh() override;
	void _generate_voxel_mesh();
	void _generate_solid_cube();

	// Private Functions

	void _generate_collider() override;
};
} // namespace godot
