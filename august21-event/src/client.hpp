#pragma once
#include <godot_cpp/classes/os.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/display_server.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/web_socket_peer.hpp>
#include <godot_cpp/variant/packed_byte_array.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <dataproto_cpp/dataproto.hpp>

using namespace godot;
using namespace dataproto;

// Global autoload singleton for client, manages global
// client state, like handling websocket connection and settings
class Client : public Node {
	GDCLASS(Client, Node);

private:
	OS *_os;
	Engine *_engine;
	DisplayServer *_display_server;
	Ref<WebSocketPeer> _socket;
	bool _socket_closed;
	void init_socket_client(String url);
	vector<PackedByteArray> poll_next_packets();

protected:
	static void _bind_methods();

public:
	Client();
	~Client();
	void _ready() override;
	void _process(double delta) override;
	Ref<WebSocketPeer> get_socket();
	Error send(BufWriter* packet);
	Error send(const char* data, size_t size);
};
