#include <cstdint>
#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/variant/signal.hpp>
#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/classes/web_socket_peer.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <dataproto_cpp/dataproto.hpp>

#include "loading_screen.hpp"
#include "network_manager.hpp"
#include "network_shared.hpp"

using namespace std;
using namespace godot;
using namespace dataproto;
using namespace NetworkShared;

LoadingScreen::LoadingScreen() : _network_manager(nullptr)
{
}

LoadingScreen::~LoadingScreen()
{
}

void LoadingScreen::_bind_methods()
{
}

void LoadingScreen::_ready()
{
	_os = OS::get_singleton();
	_engine = Engine::get_singleton();
	_display_server = DisplayServer::get_singleton();
	bool is_server = _os->has_feature("dedicated_server") || _display_server->get_name() == "headless";
	if (_engine->is_editor_hint() || is_server) {
		return;
	}
	UtilityFunctions::print("Starting as client...");

	_network_manager = (NetworkManager*) _engine->get_singleton("NetworkManager");
	if (_network_manager == nullptr) {
		UtilityFunctions::printerr("Could not initialise network manager: singleton was null");
	}

	_players_label = get_node<Label>("%PlayerCountLabel");
	_network_manager->init_client("ws://localhost:8082");
}

void LoadingScreen::_process(double delta)
{
	if (_engine->is_editor_hint()) {
		return;
	}

	if (_network_manager != nullptr) {
		auto packets = _network_manager->poll_next_packets();
		for (BufReader packet : packets) {
			uint8_t code = packet.u8();
			switch (code) {
				case ServerPacket::LINK_RESPONSE: {
					break;
				}
				case ServerPacket::GAME_INFO: {
					auto player_count = packet.u32();
					auto formatted_count = String("Players waiting: {0}")
						.format(Array::make(player_count));
					_players_label->set_text(formatted_count);
					break;
				}
				case ServerPacket::SET_PHASE: {
					auto phase_name_str = (string) packet.str();
					auto phase_name = String(phase_name_str.c_str());
					if (phase_name == "roof") {
						get_tree()->change_scene_to_file("res://scenes/roof.tscn");
					}
					else {
						UtilityFunctions::printerr("Could not set phase to ", phase_name, ": unknown phase");
					}
					break;
				}
			}
		}
	}
}
