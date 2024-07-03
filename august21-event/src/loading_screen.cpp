#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/variant/signal.hpp>
#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/classes/web_socket_peer.hpp>
#include <dataproto_cpp/dataproto.hpp>

#include "loading_screen.hpp"
#include "network_manager.hpp"

using namespace godot;
using namespace dataproto;
using namespace NetworkManager;

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
	_engine = _engine->get_singleton();
	if (_engine->is_editor_hint()) {
		return;
	}
	_players_label = get_node<Label>("%PlayerCountLabel");
	NetworkManager::instantiate();
}

void LoadingScreen::_process(double delta)
{
	if (_engine->is_editor_hint()) {
		return;
	}

	auto packets = poll_next_packets();
	for (BufReader packet : packets) {
		uint8_t code = packet.u8();
		switch (code) {
			case ServerPacket::CONFIGURATION: {
				break;
			}
			case ServerPacket::LOADING_INFO: {
				uint32_t waiting_count = packet.u32();
				auto formatted_count = String("Players waiting: {0}")
					.format(Array::make(waiting_count));
				//_players_label->set_text(formatted_count);
				break;
			}
			case ServerPacket::START: {
				UtilityFunctions::print("Starting event..");
				break;
			}
		}
	}
}