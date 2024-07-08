#pragma once
#include <godot_cpp/classes/web_socket_peer.hpp>
#include <dataproto_cpp/dataproto.hpp>

using namespace godot;
using namespace dataproto;

namespace NetworkManager {
	enum ServerPacket {
		CONFIGURATION = 0,
		LOADING_INFO = 1,
		START = 2,
		ENTITY_CREATE = 16,
		ENTITY_UPDATE = 17,
		ENTITY_DELETE = 18
	};

	void init_client(String url);
	vector<BufReader> poll_next_packets();
}