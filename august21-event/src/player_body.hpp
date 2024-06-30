#pragma once
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/input.hpp>
#include <godot_cpp/classes/input_event.hpp>
#include <godot_cpp/variant/vector3.hpp>
#include <godot_cpp/classes/character_body3d.hpp>
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/classes/camera3d.hpp>

using namespace godot;

class PlayerBody : public CharacterBody3D {
	GDCLASS(PlayerBody, CharacterBody3D);

private:
	Engine* _engine;
	Input* _player_input;
	ProjectSettings* _project_settings;
	float _gravity;
	Camera3D* _camera;
	Vector3 _velocity;

protected:
	static void _bind_methods();

public:
	PlayerBody();
	~PlayerBody();
	void _ready() override;
	void _input(const Ref<InputEvent> &event) override;
	void _physics_process(double delta) override;
};