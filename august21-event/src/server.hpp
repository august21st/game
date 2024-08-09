#pragma once
#include "dataproto_cpp/dataproto.hpp"
#include <godot_cpp/templates/list.hpp>
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/web_socket_multiplayer_peer.hpp>
#include <godot_cpp/classes/os.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/display_server.hpp>
#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/classes/thread.hpp>
#include <godot_cpp/templates/hash_map.hpp>

#include "client_data.hpp"

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
	Ref<Thread> _console_thread;
	ResourceLoader* _resource_loader;
	double _game_time;
	int _tick_count;
	HashMap<String, Node*> _phase_scenes;
	HashMap<int, ClientData*> _clients;
	List<Node*> _entities;
	Error register_phase_scene(String identifier, String path);
	template<typename T>
	Error load_scene(String scene_path, T** out_scene_instance);
	void orphan_node(Node* node);

protected:
	static void _bind_methods();

public:
	Server();
	~Server();

	void _ready() override;
	void _physics_process(double delta) override;
	void _on_peer_connected(int id);
	void _on_peer_disconnected(int id);
	void run_console_loop();
	void set_phase(string name);
	void create_entity(string type);
	void update_entity(int id, string property, string value);
	void delete_entity(int id);
	void list_players();
	void kill_player(int id);
	void kick_player(int id);
	void tp_player(string scene, int x, int y, int z);
	void announce(string message);
	void send_to_all(BufWriter* packet);
	void send_to_all(const char* data, size_t size);
	void send(int id, BufWriter* packet);
	void send(int id, const char* data, size_t size);
};
