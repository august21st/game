#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/variant/signal.hpp>
#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/classes/web_socket_peer.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/classes/window.hpp>
#include <dataproto_cpp/dataproto.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/callable.hpp>

#include "client.hpp"
#include "loading_screen.hpp"
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
	ClassDB::bind_method(D_METHOD("_on_packet_received", "packed_packet"),
		&LoadingScreen::_on_packet_received);
}

void LoadingScreen::_ready()
{
	_engine = Engine::get_singleton();
	if (_engine && _engine->is_editor_hint()) {
		return;
	}

	_client = get_tree()->get_root()->get_node<Client>("/root/GlobalClient");
	if (_client == nullptr) {
		UtilityFunctions::printerr("Could not get client: autoload singleton was null");
		return;
	}
	_client->connect("packet_received", Callable(this, "_on_packet_received"));

	_players_label = get_node<Label>("%PlayerCountLabel");
}

void LoadingScreen::_on_packet_received(PackedByteArray packed_packet)
{
	auto packet = BufReader((char*) packed_packet.ptr(), packed_packet.size());
	uint8_t code = packet.u8();
	switch (code) {
		case ServerPacket::GAME_INFO: {
			auto player_count = packet.u32();
			auto formatted_count = String("Players waiting: {0}")
				.format(Array::make(player_count));
			_players_label->set_text(formatted_count);
			break;
		}
	}
}
