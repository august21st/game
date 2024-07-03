#pragma once
#include <godot_cpp/classes/web_socket_peer.hpp>
#include <dataproto_cpp/dataproto.hpp>

using namespace godot;
using namespace dataproto;

namespace NetworkManager {
	enum ServerPacket {
		CONFIGURATION = 0,
		LOADING_INFO = 1,
		START = 2
	};

	void instantiate();
	vector<BufReader> poll_next_packets();
}