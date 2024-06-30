#include <godot_cpp/classes/engine.hpp>
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
    _engine = Engine::get_singleton();
    if (_engine->is_editor_hint())
    {
        return;
    }
    _building_camera_player = Object::cast_to<AnimationPlayer>(find_child("BuildingCameraPlayer"));
    _building_camera_player->play("intro_animation");
}