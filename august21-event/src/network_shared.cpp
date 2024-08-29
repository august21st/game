#include "network_shared.hpp"
#include <godot_cpp/classes/os.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/display_server.hpp>

using namespace godot;


namespace NetworkShared {
	bool is_server()
	{
		auto os = OS::get_singleton();
		auto engine = Engine::get_singleton();
		auto display_server = DisplayServer::get_singleton();
		return os->has_feature("dedicated_server") || display_server->get_name() == "headless"
			|| os->get_cmdline_args().has("--server");
	}
}
