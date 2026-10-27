#pragma once

#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/core/math.hpp>

namespace godot {

class VoxelCube : public Node3D {
	GDCLASS(VoxelCube, Node3D)

private:
	Vector3i size;
	std::unique_ptr<uint8_t[]> voxels;

protected:
	static void _bind_methods();

public:
	VoxelCube();
	~VoxelCube();

	void _ready() override;
	void _process(double delta) override;

	void set_size(const Vector3i newSize);
	Vector3i get_size() const { return size; }
};

} // namespace godot
