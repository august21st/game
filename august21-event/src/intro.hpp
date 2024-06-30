#pragma once
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/classes/animation_player.hpp>

using namespace godot;

class Intro : public Node3D {
	GDCLASS(Intro, Node3D);

protected:
	Engine* _engine;
    AnimationPlayer* _building_camera_player;
	static void _bind_methods();

public:
	Intro();
	~Intro();
	void _ready() override;
};