extends Node3D


@export var ray_one_collision_per_cast: bool = true
@export var debug_draw_mode: Viewport.DebugDraw = 0 as Viewport.DebugDraw :
	set(value):
		debug_draw_mode = value
		get_viewport().debug_draw = debug_draw_mode
		if (debug_draw_mode == Viewport.DebugDraw.DEBUG_DRAW_WIREFRAME):
			RenderingServer.set_debug_generate_wireframes(debug_draw_mode)
		else:
			RenderingServer.set_debug_generate_wireframes(false)
# Variables that will be passed into fracture propagation start function
@export var voxel_terrain: VoxelTerrain
#@export var impact_force: float = 1.0f
#@export var impacted_speed_metres_per_second: float = 1.0f

var _left_mouse_held: bool = false
var _ray: RayCast3D
var _ray_collided: bool = false

const RAY_CAST_SIZE: float = 100

@onready var camera: Camera3D = $Core/Camera3D


# Called when the node enters the scene tree for the first time.
func _ready() -> void:
	_ray = RayCast3D.new()
	add_child(_ray)


# Called every frame. 'delta' is the elapsed time since the previous frame.
func _process(_delta: float) -> void:
	# Draw UI
	_draw_imgui()

	# Check if the raycast has collided with any of the Voxel nodes
	if _ray.is_colliding() and !_ray_collided:
		var collided_object = _ray.get_collider()
		var collision_point = _ray.get_collision_point()
		if (collided_object):
			if (ray_one_collision_per_cast):
				_ray_collided = true
			# NOTE: Make sure the MeshInstance3D node is placed at position 0,0,0 of the VoxelCube node.
			var collision_point_in_local_space: Vector3 = collided_object.to_local(collision_point)
			#print ("_ray Collision Point: ", "%0.3v" % collision_point);
			#print ("_ray Collision Position in Local Space: ", "%0.3v" % collision_point_in_local_space);

			# Nudge the collision into the voxel so that the voxel position can then be found by rounding
			# Otherwise it will be on the exact boundary of a voxel and will be unclear which voxel it hit.
			collision_point_in_local_space -= _ray.get_collision_normal() * 0.1
			var voxel_position: Vector3 = collision_point_in_local_space.round()
			print ("Voxel Position: ", "%0.3v" % voxel_position);
			if collided_object is VoxelCube:
				#print ("VoxelCube detected")
				var collided_voxel_cube = collided_object as VoxelCube
				collided_voxel_cube.remove_voxel(voxel_position)
			elif collided_object is VoxelTerrain:
				#print ("VoxelTerrain detected")
				var collided_voxel_terrain = collided_object as VoxelTerrain
				collided_voxel_terrain.remove_voxel(voxel_position)

	# Create raycast
	if !_left_mouse_held and Input.is_action_pressed("CreateRaycastAtMouse"):
		_left_mouse_held = true
		#print("Creating Raycast")
		_create_raycast()
	if Input.is_action_just_released("CreateRaycastAtMouse"):
		_left_mouse_held = false;


# NOTE: You can enable in the debug options a way to see the raycast! It turns red on collision
# Creates a raycast at the mouse position, with direction projected from the camera
func _create_raycast() -> void:
	_ray_collided = false;
	# https://docs.godotengine.org/en/4.4/tutorials/math/matrices_and_transforms.html
	var mouse_pos = get_viewport().get_mouse_position()
	_ray.target_position = to_local(camera.project_ray_normal(mouse_pos) * RAY_CAST_SIZE)
	_ray.global_position = to_global(camera.project_ray_origin(mouse_pos))


# In-App UI for controlling the scene
func _draw_imgui() -> void:
	ImGui.Begin("Debug Window")

	var current_mode_array: Array = [debug_draw_mode]
	if(ImGui.Combo("Debug Draw Mode", current_mode_array, ["Disabled", "Unshaded", "Overdraw", "Wireframe"])):
		debug_draw_mode = current_mode_array[0] as Viewport.DebugDraw

	# Voxel Terrain
	ImGui.Text("Voxel Terrain")
	if(ImGui.Button("Generate Solid Cube")):
		voxel_terrain.create_solid_cube_SVO();
		voxel_terrain.mesh_needs_generated = true
	if(ImGui.Button("Generate Noise Voxels")):
		voxel_terrain.create_noise_terrain_SVO();
		voxel_terrain.mesh_needs_generated = true
	var data_array: Array = [voxel_terrain.noise_voxel_spawn_threshold]
	if(ImGui.SliderFloat("Voxel Noise Threshold", data_array, -1.0, 1.0)):
		voxel_terrain.noise_voxel_spawn_threshold = data_array[0]
	data_array = [voxel_terrain.noise_type]
	if(ImGui.Combo("Noise Type", data_array, ["Simplex", "Simplex Smooth", "Cellular", "Perlin", "Value Cubic", "Value"])):
		voxel_terrain.noise_type = data_array[0] as FastNoiseLite.NoiseType

	ImGui.End()
