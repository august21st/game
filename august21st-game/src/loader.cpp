#include <godot_cpp/classes/os.hpp>
#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/classes/display_server.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/classes/window.hpp>
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

#include "loader.hpp"
#include "node_shared.hpp"

using namespace NodeShared;

Loader::Loader()
{
}

Loader::~Loader()
{
}

void Loader::_bind_methods()
{
}

void Loader::_ready()
{
	auto os = OS::get_singleton();
	auto resource_loader = ResourceLoader::get_singleton();
	auto display_server = DisplayServer::get_singleton();
	auto is_server = os->has_feature("dedicated_server") || display_server->get_name() == "headless"
		|| os->get_cmdline_args().has("--server");
	auto scene_path = is_server ? "res://scenes/server.tscn" : "res://scenes/client.tscn";
	auto load_error = get_tree()->change_scene_to_file(scene_path);
	if (load_error != Error::OK) {
		UtilityFunctions::push_error("Failed to load game, root scene ", scene_path, " failed to load, with ", load_error);

		// Exit
		get_tree()->get_root()->call_deferred("propagate_notification", NOTIFICATION_WM_CLOSE_REQUEST);
		get_tree()->quit(1);
	}
}
