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

#include "client.hpp"
#include "end.hpp"

using namespace godot;
using namespace dataproto;

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
}

void End::_ready()
{
	_engine = Engine::get_singleton();
	if (_engine->is_editor_hint()) {
		return;
	}

	_client = get_tree()->get_root()->get_node<Client>("/root/GlobalClient");
	if (_client == nullptr) {
		UtilityFunctions::printerr("Could not get client: autoload singleton was null");
		return;
	}
	_client->connect("graphics_quality_changed", Callable(this, "_on_graphics_quality_changed"));

	_sun_light = get_node<DirectionalLight3D>("%SunLight");
}

void End::_on_graphics_quality_changed(int level)
{
	if (level == 0) {
		_sun_light->set_shadow(false);
	}
	else if (level == 1) {
		_sun_light->set_shadow(true);
	}
}

void End::spawn_player(PlayerBody* player)
{
	add_child(player);
	player->set_position(Vector3(0, 0, 0));
	player->set_spawn_position(Vector3(0, 0, 0));
	player->set_climbing(false);
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




