#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/variant/packed_byte_array.hpp>

#include "client.hpp"
#include "network_shared.hpp"

using namespace godot;
using namespace dataproto;
using namespace NetworkShared;

Client::Client() : _socket(Ref<WebSocketPeer>()), _socket_closed(true)
{
}

Client::~Client()
{
}

void Client::_bind_methods()
{
	ADD_SIGNAL(MethodInfo("packet_received",
		PropertyInfo(Variant::PACKED_BYTE_ARRAY, "packed_packet")));
}

void Client::_ready()
{
	_os = OS::get_singleton();
	_engine = Engine::get_singleton();
	_display_server = DisplayServer::get_singleton();
	bool is_server = _os->has_feature("dedicated_server") || _display_server->get_name() == "headless";
	if ((_engine &&_engine->is_editor_hint()) || is_server) {
		set_process(false);
		return;
	}
	UtilityFunctions::print("Starting as client...");

	init_socket_client("ws://localhost:8082");
	set_process(true);
}


void Client::init_socket_client(String url)
{
	_socket = Ref<WebSocketPeer>();
	_socket.instantiate();
	_socket->connect_to_url(url);
	set_fail_function([](const void *reason)
	{
		auto str_reason = static_cast<const char*>(reason);
		UtilityFunctions::printerr(String("dataproto error: {0}").format(Array::make(str_reason)));
	});
	_socket_closed = false;
}

Ref<WebSocketPeer> Client::get_socket()
{
	return _socket;
}

Error Client::send(BufWriter* packet)
{
	send(packet->data(), packet->size());
	return Error::OK;
}

Error Client::send(const char* data, size_t size)
{
	if (!_socket.is_valid()) {
		return Error::ERR_UNAVAILABLE;
	}

	PackedByteArray packed_data;
    packed_data.resize(size);
    memcpy(packed_data.ptrw(), data, size);
    _socket->put_packet(packed_data);
    return Error::OK;
}

vector<PackedByteArray> Client::poll_next_packets()
{
	auto packets = vector<PackedByteArray>();
	if (_socket_closed || !_socket.is_valid()) {
		return packets;
	}

	_socket->poll();
	auto state = _socket->get_ready_state();
	switch (state) {
		case WebSocketPeer::STATE_CONNECTING:
			break;
		case WebSocketPeer::STATE_OPEN:
			while (_socket->get_available_packet_count()) {
				auto packet = _socket->get_packet();
				packets.push_back(packet);
			}
			break;
		case WebSocketPeer::STATE_CLOSING:
			break;
		case WebSocketPeer::STATE_CLOSED:
			auto code = _socket->get_close_code();
			auto reason = _socket->get_close_reason();
			auto formatted_log = String("WebSocket closed with code: {0}, reason \"{1}\". Clean: {2}")
				.format(Array::make(code, reason, code != -1));
			UtilityFunctions::printerr(formatted_log);
			_socket_closed = true;
			break;
	}

	return packets;
}

void Client::_process(double delta)
{
	if (_socket_closed || !_socket.is_valid()) {
		return;
	}

	auto packed_packets = poll_next_packets();
	for (PackedByteArray packed_packet : packed_packets) {
		auto packet = BufReader((char*) packed_packet.ptr(), packed_packet.size());
		uint8_t code = packet.u8();
		switch (code) {
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
			default: {
				emit_signal("packet_received", packed_packet);
				break;
			}
		}
	}
}

