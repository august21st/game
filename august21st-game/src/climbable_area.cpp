#include <godot_cpp/classes/ref.hpp>
#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/variant/signal.hpp>

#include "climbable_area.hpp"
#include "player_body.hpp"

using namespace godot;

ClimbableArea::ClimbableArea()
{
}

ClimbableArea::~ClimbableArea()
{
}

void ClimbableArea::_bind_methods()
{
	ClassDB::bind_method(D_METHOD("_on_body_entered", "body"),
		&ClimbableArea::_on_body_entered);
	ClassDB::bind_method(D_METHOD("_on_body_exited", "body"),
		&ClimbableArea::_on_body_exited);

}

void ClimbableArea::_ready()
{
	connect("body_entered", Callable(this, "_on_body_entered"));
	connect("body_exited", Callable(this, "_on_body_exited"));
}

void ClimbableArea::_on_body_entered(Node3D* body)
{
	if (!body->is_class("PlayerBody")) {
		return;
	}
	auto player_body = Object::cast_to<PlayerBody>(body);
	player_body->set_climbing(true);
}

void ClimbableArea::_on_body_exited(Node3D* body)
{
	if (!body->is_class("PlayerBody")) {
		return;
	}
	auto player_body = Object::cast_to<PlayerBody>(body);
	player_body->set_climbing(false);
}
