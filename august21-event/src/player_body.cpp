#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/input.hpp>
#include <godot_cpp/classes/input_event.hpp>
#include <godot_cpp/classes/input_event_mouse_motion.hpp>
#include <godot_cpp/classes/input_event_mouse_button.hpp>
#include <godot_cpp/classes/input_event_action.hpp>
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/variant/vector3.hpp>
#include <godot_cpp/classes/camera3d.hpp>
#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/classes/button.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

#include "macros.hpp"
#include "player_body.hpp"

using namespace godot;

#define MAX_SPEED 6.0f
#define ACCELERATION 12.0f
#define JUMP_SPEED 5.0f
#define DECELERATION 10.0f
#define MOUSE_SENSITIVITY 0.001f
#define PI 3.14159265358979f


PlayerBody::PlayerBody()
{
}

PlayerBody::~PlayerBody()
{
}

void PlayerBody::_bind_methods()
{
}

void PlayerBody::_ready()
{
	_engine = Engine::get_singleton();
	if (_engine->is_editor_hint())
	{
		return;
	}
	_player_input = Input::get_singleton();
	_project_settings = ProjectSettings::get_singleton();
	_gravity = _project_settings->get_setting("physics/3d/default_gravity");
	_camera = get_node<Camera3D>("%PlayerCamera");
	_death_panel = get_node<Control>("%DeathPanel");
	_grab_button = get_node<Button>("%ReviveButton");
	_revive_button = get_node<Button>("%ReviveButton");
	_thumbstick_button = get_node<Button>("%ThumbstickButton");
	_velocity = Vector3(0, 0, 0);
	set_process_input(true);
	set_process(true);
}

void PlayerBody::_input(const Ref<InputEvent> &event)
{
	if (_engine->is_editor_hint()) {
		return;
	}

	if (event->is_class(nameof(InputEventMouseButton))) {
		const Ref<InputEventMouseButton> event_mouse_button = event;
		_player_input->set_mouse_mode(godot::Input::MOUSE_MODE_CAPTURED);
	}
	else  if (event->is_class(nameof(InputEventMouseMotion))) {
		const Ref<InputEventMouseMotion> event_mouse_motion = event;
		auto mouse_relative = event_mouse_motion->get_relative();

		auto body_rotation = get_rotation();
		body_rotation.y -= mouse_relative.x * MOUSE_SENSITIVITY;
		set_rotation(body_rotation);
		
		auto camera_rotation = _camera->get_rotation();
		camera_rotation.x -= mouse_relative.y * MOUSE_SENSITIVITY;
		camera_rotation.x = Math::clamp(camera_rotation.x, -PI / 2.0f, PI / 2.0f);
		_camera->set_rotation(camera_rotation);
	}
	else if (event->is_class(nameof(InputEventAction))) {
		const Ref<InputEventAction> event_action = event;
		// Release pointer lock
		if (event_action->is_action("unfocus")) {
			_player_input->set_mouse_mode(godot::Input::MOUSE_MODE_VISIBLE);
		}
	}
}

void PlayerBody::_physics_process(double delta)
{
	if (_engine->is_editor_hint()) {
		return;
	}

	// Gravity
	_velocity.y -= delta * _gravity;

	// Movement
	auto direction = Vector3();
	direction.x = _player_input->get_action_strength("move_right") - _player_input->get_action_strength("move_left");
	direction.z = _player_input->get_action_strength("move_back") - _player_input->get_action_strength("move_forward");
	// Move in direction we are looking
	direction.rotate(Vector3(0, 1, 0), get_rotation().y);
	direction.normalize();

	auto horizontal_velocity = _velocity;
	horizontal_velocity.y = 0;

	auto target = direction * MAX_SPEED;
	float acceleration;
	if (direction.dot(horizontal_velocity) > 0) {
		acceleration = ACCELERATION;
	}
	else {
		acceleration = DECELERATION;
	}

	// Apply horizontal velocity
	horizontal_velocity = horizontal_velocity.lerp(target, ACCELERATION * delta);
	_velocity.x = horizontal_velocity.x;
	_velocity.z = horizontal_velocity.z;

	// Jump
	if (is_on_floor()) {
		if (_player_input->is_action_pressed("jump")) {
			_velocity.y = JUMP_SPEED;
		}
		else {
			_velocity.y = 0;
		}
	}

	set_velocity(_velocity);
	move_and_slide();
}