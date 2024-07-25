#include <godot_cpp/variant/utility_functions.hpp>
#include <dataproto_cpp/dataproto.hpp>

#include "network_manager.hpp"
#include "godot_cpp/classes/web_socket_peer.hpp"

using namespace godot;
using namespace dataproto;

NetworkManager::NetworkManager() : _closed(true), _socket(Ref<WebSocketPeer>())
{
}

NetworkManager::~NetworkManager()
{
}

void NetworkManager::_bind_methods()
{
}

void NetworkManager::init_client(String url)
{
	_socket = Ref<WebSocketPeer>();
	_socket.instantiate();
	_socket->connect_to_url(url);
	set_fail_function([](const void *reason)
	{
		auto str_reason = static_cast<const char*>(reason);
		UtilityFunctions::printerr(String("dataproto error: {0}").format(Array::make(str_reason)));
	});
	_closed = false;
}

Ref<WebSocketPeer> NetworkManager::get_socket()
{
	return _socket;
}

Error NetworkManager::send(BufWriter* packet)
{
	send(packet->data(), packet->size());
	return Error::OK;
}

Error NetworkManager::send(const char* data, size_t size)
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

vector<BufReader> NetworkManager::poll_next_packets()
{
	auto packets = vector<BufReader>();
	if (_closed || !_socket.is_valid()) {
		return packets;
	}

	_socket->poll();
	auto state = _socket->get_ready_state();
	switch (state) {
		case WebSocketPeer::STATE_CONNECTING:
			break;
		case WebSocketPeer::STATE_OPEN:
			while (_socket->get_available_packet_count()) {
				auto packed_array = _socket->get_packet();
				auto packet = BufReader((char*) packed_array.ptr(), packed_array.size());
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
			_closed = true;
			break;
	}

	return packets;
}
