#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/variant/signal.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/variant/callable.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/method_bind.hpp>
#include <dataproto_cpp/dataproto.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/classes/window.hpp>
#include <godot_cpp/classes/directional_light3d.hpp>
#include <godot_cpp/classes/world_environment.hpp>

#include "end.hpp"
#include "entity_player_base.hpp"
#include "node_shared.hpp"

using namespace godot;
using namespace dataproto;
using namespace NodeShared;

End::End()
{
}

End::~End()
{
}

void End::_bind_methods()
{
	ClassDB::bind_method(D_METHOD("_on_graphics_quality_changed", "level"),
		&End::_on_graphics_quality_changed);
	ClassDB::bind_method(D_METHOD("run_phase_event", "phase_event"),
		&End::run_phase_event);
}

void End::_ready()
{
	_game_root = get_game_root(this);
	if (_game_root->is_client()) {
		_game_root->connect("graphics_quality_changed", Callable(this, "_on_graphics_quality_changed"));
	}

	_sun_light = get_node<DirectionalLight3D>("%SunLight");
	_world_environment = get_node<WorldEnvironment>("%WorldEnvironment");
}

void End::_on_graphics_quality_changed(int level)
{
	if (level == 0) {
		_sun_light->set_shadow(false);
		set_environment(_world_environment, "res://assets/end_environment_low.tres");
	}
	else if (level == 1) {
		_sun_light->set_shadow(true);
		set_environment(_world_environment, "res://assets/end_environment_high.tres");
	}
}

void End::spawn_player(EntityPlayerBase* player)
{
	add_child(player);
	player->set_position(Vector3(0, 0, 0));
	if (player->is_class("PlayerBody")) {
		auto player_body = Object::cast_to<PlayerBody>(player);
		player_body->set_spawn_position(Vector3(0, 0, 0));
		player_body->set_climbing(false);
	}
}

void End::run_phase_event(String phase_event)
{
	if (phase_event == "intro") {
		return;
	}

	if (phase_event == "sandbox") {
		return;
	}
}
