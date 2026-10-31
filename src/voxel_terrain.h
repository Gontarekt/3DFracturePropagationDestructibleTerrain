#pragma once

#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/classes/static_body3d.hpp>

namespace godot {

class VoxelTerrain : public StaticBody3D {
	GDCLASS(VoxelTerrain, StaticBody3D)

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
