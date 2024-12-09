#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/variant/signal.hpp>
#include <godot_cpp/classes/animation_player.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

#include "intro.hpp"

using namespace godot;

Intro::Intro()
{
}

Intro::~Intro()
{
}

void Intro::_bind_methods()
{
}

void Intro::_ready()
{
	_building_camera_player = get_node<AnimationPlayer>("BuildingCameraPlayer");
	_building_camera_player->play("intro_animation");
}
