#include <godot_cpp/classes/os.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/display_server.hpp>
#include <dataproto_cpp/dataproto.hpp>

#include "network_shared.hpp"

using namespace dataproto;
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

	void write_vector3(BufWriter& packet, Vector3 vector)
	{
		packet.f32(vector.x);
		packet.f32(vector.y);
		packet.f32(vector.z);
	}

	Vector3 read_vector3(BufReader& packet)
	{
		auto x = packet.f32();
		auto y = packet.f32();
		auto z = packet.f32();
		return Vector3(x, y, z);
	}
}
