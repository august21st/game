#pragma once
#include "dataproto_cpp/dataproto.hpp"
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/web_socket_multiplayer_peer.hpp>
#include <godot_cpp/classes/os.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/display_server.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <map>

using namespace godot;
using namespace std;
using namespace dataproto;

class Server : public Node {
	GDCLASS(Server, Node)

private:
	OS *_os;
	Engine *_engine;
	DisplayServer *_display_server;
	Ref<WebSocketMultiplayerPeer> _socket_server;
	double _game_time;
	map<int, Ref<WebSocketPeer>> _clients;

protected:
	static void _bind_methods();

public:
	Server();
	~Server();

	void _ready() override;
	void _physics_process(double delta) override;
	void _on_client_connected(int id);
	void _on_client_disconnected(int id);
	void _on_data_received(int id, const PackedByteArray &data);
	void send_to_all(BufWriter packet);
	void send_to_all(const char* data, size_t size);
	void send(int id, BufWriter packet);
	void send(int id, const char* data, size_t size);
};
