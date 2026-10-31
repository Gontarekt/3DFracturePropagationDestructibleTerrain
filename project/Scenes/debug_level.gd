extends Node3D

var left_mouse_held: bool = false
var ray: RayCast3D
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

	if !left_mouse_held and Input.is_action_pressed("CreateRaycastAtMouse"):
		left_mouse_held = true
		print("Creating Raycast")
		_create_raycast()
	if Input.is_action_just_released("CreateRaycastAtMouse"):
		left_mouse_held = false;

# NOTE: You can enable in the debug options a way to see the raycast! It turns red on collision
func _create_raycast() -> void:
	# https://docs.godotengine.org/en/4.4/tutorials/math/matrices_and_transforms.html
	var mouse_pos = get_viewport().get_mouse_position()
	ray.target_position = to_local(camera.project_ray_normal(mouse_pos)*100)
	ray.global_position = to_global(camera.project_ray_origin(mouse_pos))
	# if ray.is_colliding:
	# 	var collision_point = ray.get_collision_point()
	# 	var collided_object = ray.get_collider()
	# 	if (collided_object):
	# 		print ("Ray Collision Point: ", collision_point);
	# 		var collided_object_transform: Transform3D = collided_object.transform.inverse() # I believe this is the equivalent of world matrices in godot.
	# 		var local_space_transform: Transform3D = ray.transform * collided_object_transform
	# 		ray.transform = local_space_transform
	# 		print ("Ray Position in Local Space: ", ray.global_position);
