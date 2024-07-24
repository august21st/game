#include <godot_cpp/classes/web_socket_multiplayer_peer.hpp>
#include <godot_cpp/classes/web_socket_peer.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/classes/thread.hpp>
#include <godot_cpp/templates/hash_map.hpp>
#include <dataproto_cpp/dataproto.hpp>
#include <commandIO.hpp>

#include "server.hpp"
#include "godot_cpp/core/class_db.hpp"
#include "godot_cpp/variant/packed_byte_array.hpp"
#include "network_shared.hpp"

using namespace commandIO;
using namespace dataproto;
using namespace godot;
using namespace std;
using namespace NetworkShared;

const int SERVER_PORT = 8082;
const int TPS = 20;

Server::Server() : _socket_server(nullptr)
{
}

Server::~Server()
{
	if (_socket_server.is_valid()) {
		_socket_server->disconnect("peer_connected", Callable(this, "_on_peer_connected"));
		_socket_server->disconnect("peer_disconnected", Callable(this, "_on_peer_disconnected"));
		_socket_server->close();
	}
}

void Server::_bind_methods()
{
	ClassDB::bind_method(D_METHOD("_on_peer_connected", "id"), &Server::_on_peer_connected);
	ClassDB::bind_method(D_METHOD("_on_peer_disconnected", "id"), &Server::_on_peer_disconnected);
	ClassDB::bind_method(D_METHOD("run_console_loop"), &Server::run_console_loop);
}

void Server::_ready()
{
	_os = OS::get_singleton();
	_engine = Engine::get_singleton();
	_display_server = DisplayServer::get_singleton();
	bool is_server = _os->has_feature("dedicated_server") || _display_server->get_name() == "headless";
	if (_engine->is_editor_hint() || !is_server) {
		set_physics_process(false);
		return;
	}
	UtilityFunctions::print("Starting as server...");

	_console_thread = Ref<Thread>();
	_console_thread.instantiate();
	_console_thread->start(Callable(this, "run_console_loop"));

	_socket_server = Ref<WebSocketMultiplayerPeer>();
	_socket_server.instantiate();
	_socket_server->connect("peer_connected", Callable(this, "_on_peer_connected"));
	_socket_server->connect("peer_disconnected", Callable(this, "_on_peer_disconnected"));

	auto socket_error = _socket_server->create_server(SERVER_PORT);
	if (socket_error != godot::Error::OK) {
		UtilityFunctions::printerr("Failed to start server, received error: ", socket_error);
		return;
	}
	UtilityFunctions::print("Websocket started on port ", SERVER_PORT);

	_entities = { };
	_game_time = 0.0L;
	_engine->set_physics_ticks_per_second(TPS);
	set_process(true);
	set_physics_process(true);
}

void Server::_physics_process(double delta)
{
	// Handle new WS updates
	if (_socket_server.is_valid()) {
		_socket_server->poll();
		while (_socket_server->get_available_packet_count()) {
			auto sender_id = _socket_server->get_packet_peer();
			auto iterator = _clients.find(sender_id);
			if (iterator == _clients.end()) {
				UtilityFunctions::print("Could not handle data from sender ", sender_id, ": not found in clients map");
				return;
			}
			auto sender = _clients[sender_id];
			auto data = _socket_server->get_packet();
			UtilityFunctions::print("Data received from client ", sender_id, ": ", data);
			auto packet = BufReader((char*) data.ptr(), data.size());
			auto code = packet.u8();
			switch (code) {
				case ClientPacket::AUTHENTICATE_LINKAGE: {
					auto link_key = packet.str();
					// TODO: Authenticate link key with server, set user int ID, retrieve chat name... etc
					break;
				}
				case ClientPacket::MOVEMENT_UPDATE: {
					break;
				}
				case ClientPacket::DROP_ACTION: {
					break;
				}
				case ClientPacket::GRAB_ACTION: {
					break;
				}
				case ClientPacket::CHAT_MESSAGE: {
					auto message = packet.str();
					auto chat_packet = new BufWriter();
					chat_packet->u8(ServerPacket::CLIENT_CHAT_MESSAGE);
					chat_packet->i32(0); // TODO: entity_id
					chat_packet->i32(0); // TODO: user_int_id
					chat_packet->str(message);
					send_to_all(chat_packet);
					delete chat_packet;
					break;
				}
			}
		}

		// Every 2s update player count
		if (((int) Math::floor(_game_time)) % 2 == 0) {
			auto game_info_packet = new BufWriter();
			game_info_packet->u8(ServerPacket::GAME_INFO);
			game_info_packet->u32((uint32_t) _clients.size());
			send_to_all(game_info_packet);
			delete game_info_packet;
		}
	}

	// Run game loop
	_game_time += delta;
}

void Server::_on_peer_connected(int id)
{
	Ref<WebSocketPeer> client = _socket_server->get_peer(id);
	_clients[id] = client;
	UtilityFunctions::print("Client connected: ", id);
}

void Server::_on_peer_disconnected(int id)
{
	_clients.erase(id);
	UtilityFunctions::print("Client disconnected: ", id);
}

void Server::run_console_loop()
{
	ReplIO repl;
	while (interface(repl,
		func(pack(this, &Server::set_phase), "phase", "Set the game phase to the specified stage",
			param("name", "String name of phase")),
		func(pack(this, &Server::create_entity), "create_entity", "Create a new entity of specified type",
			param("type", "Node type name of entity to be created")),
		func(pack(this, &Server::delete_entity), "delete_entity", "Delete a specific entity",
			param("id", "Id of entity to be deleted")),
		func(pack(this, &Server::update_entity), "update_entity", "Update a property of a specific entity",
			param("id", "Id of entity to be created"),
			param("property", "Name of property to be modified"),
			param("value", "New value to set of specified property")),
		func(pack(this, &Server::list_players), "list_players", "List all connecteed players"),
		func(pack(this, &Server::kill_player), "kill_player", "Kill a specific player",
			param("id", "Id of player to be killed")),
		func(pack(this, &Server::kick_player), "kick_player", "Disconnect a specific player",
			param("id", "Id of player to be disconnected")),
		func(pack(this, &Server::tp_player), "tp_player", "Teleport a specific playerx",
			param("scene", "String name of scene in which to teleport player"),
			param("x", "X-coordinate of new location"),
			param("y", "Y-coordinate of new location"),
			param("z", "Z-coordinate of new location"))));
}

void Server::set_phase(string name)
{
}

void Server::create_entity(string type)
{
}

void Server::delete_entity(int id)
{
}

void Server::update_entity(int id, string property, string value)
{
}

void Server::list_players()
{
}

void Server::kill_player(int id)
{
}

void Server::kick_player(int id)
{
}

void Server::tp_player(string scene, int x, int y, int z)
{
}

void Server::send_to_all(BufWriter* packet)
{
	send_to_all(packet->data(), packet->size());
}

void Server::send_to_all(const char* data, size_t size)
{
    PackedByteArray packed_data;
    packed_data.resize(size);
    memcpy(packed_data.ptrw(), data, size);

    for (auto &[id, client] : _clients) {
        client->put_packet(packed_data);
    }
}

void Server::send(int id, BufWriter* packet)
{
	send(id, packet->data(), packet->size());
}

void Server::send(int id, const char* data, size_t size)
{
	PackedByteArray packed_data;
    packed_data.resize(size);
    memcpy(packed_data.ptrw(), data, size);
    _clients[id]->put_packet(packed_data);
}
