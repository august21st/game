#pragma once
#include <godot_cpp/classes/input_event.hpp>
#include <godot_cpp/classes/camera3d.hpp>
#include <godot_cpp/classes/input.hpp>

using namespace godot;

class ServerCamera : public Camera3D {
	GDCLASS(ServerCamera, Camera3D);

private:
	Input* _camera_input;
	bool _right_mouse_drag;
	float _move_speed;
	const float MOUSE_SENSITIVITY = 0.04f;
	const float SCROLL_FACTOR = 1.1;
	const float MOVE_SPEED_MIN = 4.0f;
	const float MOVE_SPEED_MAX = 100.0f;

protected:
	static void _bind_methods();

public:
	ServerCamera();
	~ServerCamera();
	void _ready() override;
	void _unhandled_input(const Ref<InputEvent>& event) override;
	void _process(double delta) override;
};
