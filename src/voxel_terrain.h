#pragma once

#include <godot_cpp/classes/node3d.hpp>

namespace godot {

class VoxelTerrain : public Node3D {
	GDCLASS(VoxelTerrain, Node3D)

private:


protected:
	static void _bind_methods();

public:
	VoxelTerrain();
	~VoxelTerrain();

	void _ready() override;
	void _process(double delta) override;
};

} // namespace godot
