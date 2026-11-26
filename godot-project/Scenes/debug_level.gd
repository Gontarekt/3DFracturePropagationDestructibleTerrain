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
@export var impact_force: float = 1.0
#@export var impacted_speed_metres_per_second: float = 1.0

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
			var offset_local_space_collision_point: Vector3 = collision_point_in_local_space - (_ray.get_collision_normal() * 0.1)
			var voxel_position: Vector3i = offset_local_space_collision_point.round()
			#print ("\nGODOT RAY Voxel Position: ", "%0.3v" % voxel_position)

			if collided_object is VoxelObject:
				var collided_voxel_object = collided_object as VoxelObject
				collided_voxel_object.remove_voxel(voxel_position)
				collided_voxel_object.set_mesh_needs_generated(true)
			elif collided_object is VoxelCube:
				#print ("VoxelCube detected")
				var collided_voxel_cube = collided_object as VoxelCube
				collided_voxel_cube.remove_voxel(voxel_position)
				collided_voxel_cube.set_mesh_needs_generated(true)
			elif collided_object is VoxelTerrain:
				#print ("VoxelTerrain detected")
				var collided_voxel_terrain = collided_object as VoxelTerrain

				# If an empty voxel was hit, step through the terrain
				# Unfortunately since I had to make VoxelNode a non-godot class to support it being used for both static and rigid bodies,
				# this means that I can't just cast each collided object to VoxelNode and must repeat the same logic for each.
				# TODO: Make this a function
				if collided_voxel_terrain.get_voxel(voxel_position) == 0:
					var ray_direction_global: Vector3 = (_ray.to_global(_ray.target_position) - _ray.global_position).normalized()
					var ray_local_space_offset_position: Vector3 = collided_object.to_local(collision_point + ray_direction_global)
					var ray_direction: Vector3 = (ray_local_space_offset_position - collision_point_in_local_space).normalized()

					var collided_voxel_position: Vector3 = collision_point_in_local_space + (ray_direction * 0.1)
					# STEP_SIZE is the offset used to step through the grid, use small steps so it has less chance of skipping voxels when travelling
					# diagonally through the grid. However this will mean more steps will be needed to get through the entire grid.
					var STEP_SIZE: Vector3 = ray_direction * 0.5
					# NOTE: If the grid size changes, you'll need more steps to get through the entire grid.
					var MAX_COLLISION_STEPS : int = 150
					var found_valid_voxel: bool = false
					var collision_step : int = 0;

					while (!found_valid_voxel and collision_step < MAX_COLLISION_STEPS):
						collision_step += 1
						collided_voxel_position += STEP_SIZE
						if collided_voxel_terrain.get_voxel(collided_voxel_position.round()) != 0:
							found_valid_voxel = true;
							voxel_position = collided_voxel_position.round()

				#collided_voxel_terrain.remove_voxel(voxel_position)
				collided_voxel_terrain.initiate_fracture_propagation(voxel_position, impact_force)
				collided_voxel_terrain.set_mesh_needs_generated(true)

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

	var data_array: Array

	data_array = [debug_draw_mode]
	if(ImGui.Combo("Debug Draw Mode", data_array, ["Disabled", "Unshaded", "Overdraw", "Wireframe"])):
		debug_draw_mode = data_array[0] as Viewport.DebugDraw

	data_array = [impact_force]
	if(ImGui.InputFloat("Impact Force", data_array)):
		impact_force = data_array[0] as float

	# Voxel Terrain
	ImGui.Text("Voxel Terrain")
	if(ImGui.Button("Generate Solid Cube")):
		voxel_terrain.create_solid_cube_SVO();
		voxel_terrain.set_mesh_needs_generated(true)
	if(ImGui.Button("Generate Noise Voxels")):
		voxel_terrain.create_noise_terrain_SVO();
		voxel_terrain.set_mesh_needs_generated(true)
	if(ImGui.Button("Disconnected Voxels Scan")):
		voxel_terrain.set_check_disconnected_components(true)
	data_array = [voxel_terrain.get_draw_toughness_debug_mesh()]
	if(ImGui.Checkbox("Draw toughness debug mesh", data_array)):
		voxel_terrain.set_draw_toughness_debug_mesh(data_array[0])
	data_array = [voxel_terrain.noise_voxel_spawn_threshold]
	if(ImGui.SliderFloat("Voxel Noise Threshold", data_array, -1.0, 1.0)):
		voxel_terrain.noise_voxel_spawn_threshold = data_array[0]
	data_array = [voxel_terrain.noise_type]
	if(ImGui.Combo("Noise Type", data_array, ["Simplex", "Simplex Smooth", "Cellular", "Perlin", "Value Cubic", "Value"])):
		voxel_terrain.noise_type = data_array[0] as FastNoiseLite.NoiseType

	ImGui.End()
