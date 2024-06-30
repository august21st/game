#include <cstdint>
#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/variant/signal.hpp>
#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/classes/web_socket_peer.hpp>
#include <dataproto_cpp/dataproto.hpp>

#include "loading_screen.hpp"

using namespace godot;

#define SV_PACKET_CONFIGURATION 0
#define SV_PACKET_LOADING_INFO 1
#define SV_PACKET_START 2

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
	if (_engine->is_editor_hint())
	{
		return;
	}
	_players_label = Object::cast_to<Label>(find_child("%PlayerCountLabel"));
	_socket = Ref<WebSocketPeer>();
	_socket.instantiate();
	_socket->connect_to_url("ws://localhost:8088");
}

void LoadingScreen::_process(double delta)
{
	if (_engine->is_editor_hint())
	{
		return;
	}

	_socket->poll();
	auto state = _socket->get_ready_state();
	switch (state)
	{
		case WebSocketPeer::STATE_CONNECTING:
			break;
		case WebSocketPeer::STATE_OPEN:
			while (_socket->get_available_packet_count()) {
				auto packed_array = _socket->get_packet();
				auto packet = BufReader((char*) packed_array._native_ptr(), packed_array.size());
				uint8_t code = packet.u8();
				switch (code)
				{
					case SV_PACKET_CONFIGURATION:
						break;
					case SV_PACKET_START:
						UtilityFunctions::print("Starting event..");
						break;
					case SV_PACKET_LOADING_INFO:
						uint32_t waiting_count = packet.u32();
						auto formatted_count = String("Players waiting: {0}").format(waiting_count);
						_players_label->set_text(formatted_count);
						break;
				}
			}
			break;
		case WebSocketPeer::STATE_CLOSING:
			break;
		case WebSocketPeer::STATE_CLOSED:
			auto code = _socket->get_close_code();
			auto reason = _socket->get_close_reason();
			auto formatted_log = String("WebSocket closed with code: {0}, reason \"{1}\". Clean: {2}")
				.format(Array::make(code, reason, code != -1));
    		UtilityFunctions::push_error(formatted_log);
			set_process(false);
			break;
	}
}