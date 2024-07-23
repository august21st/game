#pragma once
#include <godot_cpp/classes/web_socket_peer.hpp>
#include <dataproto_cpp/dataproto.hpp>

using namespace godot;
using namespace dataproto;

class NetworkManager : public Object {
	GDCLASS(NetworkManager, Object)

private:
	bool _closed;
	Ref<WebSocketPeer> _socket;

protected:
	static void _bind_methods();

public:
	NetworkManager();
	~NetworkManager();
	void init_client(String url);
	bool get_closed();
	Ref<WebSocketPeer> get_socket();
	vector<BufReader> poll_next_packets();
};
