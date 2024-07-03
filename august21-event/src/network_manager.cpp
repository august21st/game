#include <godot_cpp/variant/utility_functions.hpp>
#include <dataproto_cpp/dataproto.hpp>

#include "network_manager.hpp"

using namespace godot;
using namespace dataproto;

namespace NetworkManager {
	Ref<WebSocketPeer> _socket;
	void instantiate()
	{
		_socket = Ref<WebSocketPeer>();
		_socket.instantiate();
		_socket->connect_to_url("ws://localhost:8088");
		set_fail_function([](const void *reason)
		{
			auto str_reason = static_cast<const char*>(reason);
			UtilityFunctions::printerr(String("dataproto error: {0}").format(str_reason));
		});
	}

	vector<BufReader> poll_next_packets()
	{
		_socket->poll();
		auto packets = vector<BufReader>();
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
				break;
		}

		return packets;
	}
}