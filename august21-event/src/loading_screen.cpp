#include <cstdint>
#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/variant/signal.hpp>
#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/classes/web_socket_peer.hpp>
#include <dataproto_cpp/dataproto.hpp>

#include "loading_screen.hpp"
#include "network_manager.hpp"
#include "network_shared.hpp"

using namespace std;
using namespace godot;
using namespace dataproto;
using namespace NetworkShared;

LoadingScreen::LoadingScreen()
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
	_engine = Engine::get_singleton();
	if (_engine->is_editor_hint()) {
		return;
	}

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

	auto packets = _network_manager->poll_next_packets();
	for (BufReader packet : packets) {
		uint8_t code = packet.u8();
		switch (code) {
			case ServerPacket::CONFIGURATION: {
				break;
			}
			case ServerPacket::LOADING_INFO: {
				auto waiting_count = packet.u32();
				auto formatted_count = String("Players waiting: {0}")
					.format(Array::make(waiting_count));
				_players_label->set_text(formatted_count);
				break;
			}
			case ServerPacket::START: {
				UtilityFunctions::print("loading_screen: Starting game...");
				break;
			}
		}
	}
}
