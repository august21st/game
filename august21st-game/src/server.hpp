#pragma once
#include "game_root.hpp"
#include <godot_cpp/classes/random_number_generator.hpp>
#include <godot_cpp/classes/reg_ex.hpp>
#include <godot_cpp/templates/list.hpp>
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/item_list.hpp>
#include <godot_cpp/classes/line_edit.hpp>
#include <godot_cpp/classes/web_socket_multiplayer_peer.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/classes/thread.hpp>
#include <godot_cpp/classes/time.hpp>
#include <godot_cpp/classes/mutex.hpp>
#include <godot_cpp/templates/hash_map.hpp>
#include <godot_cpp/classes/display_server.hpp>
#include <godot_cpp/classes/camera3d.hpp>
#include <godot_cpp/classes/check_button.hpp>
#include <godot_cpp/classes/button.hpp>
#include <dataproto_cpp/dataproto.hpp>

#include "game_root.hpp"
// Forward declare to fix circular dependency
class EntityItemBase;
#include "entity_item_base.hpp"
#include "client_data.hpp"
#include "entity_info.hpp"
#include "packet_info.hpp"
#include "network_shared.hpp"

using namespace godot;
using namespace dataproto;
using namespace NetworkShared;

class Server : public GameRoot {
	GDCLASS(Server, GameRoot)

private:
	Engine* _engine;
	Ref<WebSocketMultiplayerPeer> _socket_server;
	Ref<Thread> _console_thread;
	DisplayServer* _display_server;
	ResourceLoader* _resource_loader;
	Time* _time;

	Node3D* _server_scenes_container;
	List<PacketInfo*> _incoming_packets;
	bool _incoming_packets_logging;
	void set_incoming_packets_logging(bool value);
	bool get_incoming_packets_logging();
	ItemList* _incoming_packets_list;
	LineEdit* _incoming_packets_filter;
	void _on_incoming_packets_filter_text_submitted(String new_text);
	Button* _incoming_packets_filter_clear;
	void _on_incoming_packets_filter_clear_pressed();
	CheckButton* _incoming_packets_enabled;
	void _on_incoming_packets_enabled_pressed();
	bool _outgoing_packets_logging;
	void set_outgoing_packets_logging(bool value);
	bool get_outgoing_packets_logging();
	List<PacketInfo*> _outgoing_packets;
	ItemList* _outgoing_packets_list;
	LineEdit* _outgoing_packets_filter;
	void _on_outgoing_packets_filter_text_submitted(String new_text);
	Button* _outgoing_packets_filter_clear;
	void _on_outgoing_packets_filter_clear_pressed();
	CheckButton* _outgoing_packets_enabled;
	void _on_outgoing_packets_enabled_pressed();
	void packets_list_autoscroll(VScrollBar* scroll_bar);
	void eval_packet_filter(String filter, List<PacketInfo*> infos, ItemList* info_list);
	void add_incoming_packet_info(ClientPacket code, ClientData* from);
	void add_outgoing_packet_info(ServerPacket code, ClientData* to = nullptr);

	double _start_time;
	double _game_time;
	int _tick_count;
	String _current_phase_scene;
	String _current_phase_event;
	HashMap<String, Viewport*> _server_scenes;
	HashMap<String, Node*> _phase_scenes;
	HashMap<int, ClientData*> _clients;
	HashMap<int, ClientData*> _authenticated_clients;
	HashMap<int, EntityInfo*> _entities;
	Mutex* _entities_lock;
	uint _entities_head;
	RegEx* _bbcode_regex;
	Error register_phase_scene(String identifier, String path);
	int hash_string(String value);
	void write_player_info(int id, ClientData* client, BufWriter& buffer);
	uint next_entity_id();

	// Testing
	bool _fuzz_testing;
	Ref<RandomNumberGenerator> _rng;
	Ref<WebSocketPeer> _fuzz_socket;
	void initialize_fuzzing();
	void connect_fuzz_socket();
	void process_fuzzing(double delta);
	PackedByteArray generate_fuzz_packet();
	void maybe_disconnect_fuzz();
	String generate_random_string(int length);
	Vector3 generate_random_vector3();

	// Constants
	const int PLAYER_LIMIT = 512;
	const int SERVER_PORT = 8021;
	const int TPS = 20;

protected:
	static void _bind_methods();

public:
	Server();
	~Server();
	void _ready() override;
	void _physics_process(double delta) override;
	void _on_peer_connected(int id);
	void _on_peer_disconnected(int id);
	void distribute_server_info();
	void run_console_loop();
	EntityInfo* register_entity(Node* entity, String parent_scene);
	void repl_create_entity(string node_path, string parent_scene);
	// thread safe
	EntityInfo* create_entity(String node_path, String parent_scene);
	EntityInfo* get_entity(int id);
	void repl_set_phase(string name, bool unload_previous);
	void set_phase(String name, bool unload_previous = false);
	void repl_update_entity(int id, string property, string value);
	void delete_entity(int id);
	void repl_list_players();
	void list_players();
	void repl_list_entities();
	void list_entities();
	void kill_player(int id);
	void kick_client(int id);
	void tp_player(string scene, int x, int y, int z);
	void repl_announce(string message);
	// Send to all connected websocket clients
	void send_to_all(const BufWriter& packet);
	void send_to_all(const char* data, size_t size);
	// Send to all authenticated players
	void send_to_players(const BufWriter& packet);
	void send_to_players(const char* data, size_t size);
	void send_to_other_players(int exclude_id, const BufWriter& packet);
	void send_to_other_players(int exclude_id, const char* data, size_t size);
	void send(int id, const BufWriter& packet);
	void send(int id, const char* data, size_t size);
	void send(int id, PackedByteArray packed_data);

	String get_current_phase_scene() override;
	String get_current_phase_event() override;
	bool has_phase_scene(String name) override;
	Node* get_phase_scene(String name) override;
	bool is_client() override;
	bool is_server() override;
};
