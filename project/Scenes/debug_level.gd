extends Node3D

var left_mouse_held: bool = false
var ray: RayCast3D
var ray_collided: bool = false
@onready var camera: Camera3D = $Core/Camera3D

# Called when the node enters the scene tree for the first time.
func _ready() -> void:
	ray = RayCast3D.new()
	add_child(ray)


# Called every frame. 'delta' is the elapsed time since the previous frame.
func _process(_delta: float) -> void:
	ImGui.Begin("Debug Window")
	ImGui.Text("TODO: Wireframe toggle")
	ImGui.Text("TODO: Material Toughness display toggle")
	ImGui.End()

	if ray.is_colliding() and !ray_collided:
		var collided_object = ray.get_collider()
		var collision_point = ray.get_collision_point()
		if (collided_object):
			ray_collided = true
			# NOTE: Make sure the MeshInstance3D node is placed at position 0,0,0 of the VoxelCube node.
			var collision_point_in_local_space: Vector3 = collided_object.to_local(collision_point)
			#print ("Ray Collision Point: ", "%0.3v" % collision_point);
			#print ("Ray Collision Position in Local Space: ", "%0.3v" % collision_point_in_local_space);
			# Nudge the collision into the voxel so that the voxel position can then be found by rounding
			# Otherwise it will be on the exact boundary of a voxel and will be unclear which voxel it hit.
			collision_point_in_local_space -= ray.get_collision_normal() * 0.1
			var voxel_position: Vector3 = collision_point_in_local_space.round()
			print ("Voxel Position: ", "%0.3v" % voxel_position);
			if collided_object is VoxelCube:
				#print ("VoxelCube detected")
				var voxel_cube = collided_object as VoxelCube
				voxel_cube.remove_voxel(voxel_position)

	if !left_mouse_held and Input.is_action_pressed("CreateRaycastAtMouse"):
		left_mouse_held = true
		#print("Creating Raycast")
		_create_raycast()
	if Input.is_action_just_released("CreateRaycastAtMouse"):
		left_mouse_held = false;


# NOTE: You can enable in the debug options a way to see the raycast! It turns red on collision
func _create_raycast() -> void:
	ray_collided = false;
	# https://docs.godotengine.org/en/4.4/tutorials/math/matrices_and_transforms.html
	var mouse_pos = get_viewport().get_mouse_position()
	ray.target_position = to_local(camera.project_ray_normal(mouse_pos)*100)
	ray.global_position = to_global(camera.project_ray_origin(mouse_pos))
