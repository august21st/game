#pragma once
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/input.hpp>
#include <godot_cpp/classes/input_event.hpp>
#include <godot_cpp/variant/vector3.hpp>
#include <godot_cpp/classes/character_body3d.hpp>
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/classes/camera3d.hpp>
#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/classes/button.hpp>

using namespace godot;

class PlayerBody : public CharacterBody3D {
	GDCLASS(PlayerBody, CharacterBody3D);

private:
	Engine* _engine;
	ProjectSettings* _project_settings;
	Input* _player_input;
	float _gravity;
	Vector3 _velocity;
	Camera3D* _camera;
	Control* _death_panel;
	Button* _grab_button;
	Button* _revive_button;
	Button* _thumbstick_button;

protected:
	static void _bind_methods();

public:
	PlayerBody();
	~PlayerBody();
	void _ready() override;
	void _input(const Ref<InputEvent> &event) override;
	void _physics_process(double delta) override;
};