#include <godot_cpp/classes/camera3d.hpp>
#include <godot_cpp/classes/input.hpp>
#include <godot_cpp/classes/input_event.hpp>
#include <godot_cpp/classes/global_constants.hpp>
#include <godot_cpp/classes/input_event_mouse_button.hpp>
#include <godot_cpp/classes/input_event_mouse_motion.hpp>

#include "server_camera.hpp"
#include "godot_cpp/core/math.hpp"
#include "node_shared.hpp"

using namespace NodeShared;
using namespace godot;

ServerCamera::ServerCamera() : _right_mouse_drag(false)
{
}

ServerCamera::~ServerCamera()
{
}

void ServerCamera::_bind_methods()
{
}

void ServerCamera::_ready()
{
	_camera_input = Input::get_singleton();
	_camera_input->set_mouse_mode(Input::MOUSE_MODE_VISIBLE);
	_move_speed = MOVE_SPEED_MIN;
}

void ServerCamera::_unhandled_input(const Ref<InputEvent>& event)
{
	if (event->is_class("InputEventMouseButton")) {
		const Ref<InputEventMouseButton> event_mouse_button = event;
		auto pressed = event_mouse_button->is_pressed();

		switch (event_mouse_button->get_button_index()) {
			case MouseButton::MOUSE_BUTTON_RIGHT: {
				_right_mouse_drag = pressed;
				if (pressed) {
					_camera_input->set_mouse_mode(Input::MOUSE_MODE_CAPTURED);
				}
				else {
					_camera_input->set_mouse_mode(Input::MOUSE_MODE_VISIBLE);
				}
				break;
			}
			case MouseButton::MOUSE_BUTTON_WHEEL_UP: {
				if (pressed) {
					_move_speed = Math::clamp(_move_speed * SCROLL_FACTOR, MOVE_SPEED_MIN, MOVE_SPEED_MAX);
				}
				break;
			}
			case MouseButton::MOUSE_BUTTON_WHEEL_DOWN: {
				if (pressed) {
					_move_speed = Math::clamp(_move_speed / SCROLL_FACTOR, MOVE_SPEED_MIN, MOVE_SPEED_MAX);
				}
				break;
			}
			default: {
				break;
			}
		}
	}
	else if (event->is_class("InputEventMouseMotion") && _right_mouse_drag) {
		const Ref<InputEventMouseMotion> event_mouse_motion = event;
		auto mouse_delta = event_mouse_motion->get_relative() * MOUSE_SENSITIVITY;
		auto rotation = get_rotation();
		rotation.x -= mouse_delta.y * MOUSE_SENSITIVITY;
		rotation.x = Math::clamp(rotation.x, -PI / 2.0f, PI / 2.0f);
		rotation.y -= mouse_delta.x * MOUSE_SENSITIVITY;
		set_rotation(rotation);
    }

    if (_camera_input->is_action_just_pressed("unfocus")) {
    	_camera_input->set_mouse_mode(Input::MOUSE_MODE_VISIBLE);
    	_right_mouse_drag = false;
    }
}

void ServerCamera::_process(double delta)
{
	auto input_direction = Vector3(0, 0, 0);
	input_direction.x = _camera_input->get_action_strength("move_right")
		- _camera_input->get_action_strength("move_left");
	input_direction.y = _camera_input->get_action_strength("fly_up")
		- _camera_input->get_action_strength("fly_down");
	input_direction.z = _camera_input->get_action_strength("move_back")
		- _camera_input->get_action_strength("move_forward");
	input_direction.normalize();

	translate(input_direction * _move_speed * delta);
}
