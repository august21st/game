#include <godot_cpp/classes/web_socket_multiplayer_peer.hpp>
#include <godot_cpp/classes/web_socket_peer.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/classes/thread.hpp>
#include <godot_cpp/templates/hash_map.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/classes/window.hpp>
#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/classes/packed_scene.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/packed_byte_array.hpp>
#include <godot_cpp/variant/variant.hpp>
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/resource_saver.hpp>
#include <godot_cpp/classes/file_access.hpp>
#include <dataproto_cpp/dataproto.hpp>
#include <commandIO.hpp>

#include "server.hpp"
#include "network_shared.hpp"
#include "node_shared.hpp"
#include "entity_player.hpp"
#include "entity_info.hpp"
#include "roof.hpp"
#include "end.hpp"

using namespace commandIO;
using namespace dataproto;
using namespace godot;
using namespace std;
using namespace NetworkShared;
using namespace NodeShared;

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

	// Initialise websocket
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

	// Initialise scenes
	UtilityFunctions::print("Loading phase scenes...");
	_resource_loader = ResourceLoader::get_singleton();
	_phase_scenes = { };
	auto roof_err = register_phase_scene("roof", "res://scenes/roof.tscn");
	if (roof_err != godot::Error::OK) {
		return;
	}
	auto end_err = register_phase_scene("end", "res://scenes/end.tscn");
	if (end_err != godot::Error::OK) {
		return;
	}
	UtilityFunctions::print("Sucessfully loaded phase scenes");

	// Initialise CLI
	_console_thread = Ref<Thread>();
	_console_thread.instantiate();
	_console_thread->start(Callable(this, "run_console_loop"));

	// Initialise properties
	_clients = { };
	_entities = { };
	_game_time = 0.0L;
	_tick_count = 0;
	_engine->set_physics_ticks_per_second(TPS);
	set_process(true);
	set_physics_process(true);
}

void Server::_physics_process(double delta)
{
	if (_engine->is_editor_hint()) {
		return;
	}

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
			auto packet = BufReader((char*) data.ptr(), data.size());
			auto code = packet.u8();
			switch (code) {
				case ClientPacket::LINK_KEY: {
					auto link_key = packet.str();
					// TODO: Authenticate link key with server, set user int ID, retrieve chat name... etc
					break;
				}
				case ClientPacket::UPDATE_MOVEMENT: {
					auto phase_scene_str = (string) packet.str();
					auto phase_scene = String(phase_scene_str.c_str());
					if (!_phase_scenes.has(phase_scene)) {
						UtilityFunctions::print(
							"Couldn't update movement for client {0}: phase scene doesn't exist", sender_id);
						break;
					}

					// Apply updates locally
					auto player_entity = sender->get_entity();
					auto player_scene = _phase_scenes[phase_scene];
					if (player_entity->get_parent() != player_scene) {
						// Teleport serverside client to correct scene
						orphan_node(player_entity);
						player_scene->add_child(player_entity);
					}
					auto position = Vector3(packet.f32(), packet.f32(), packet.f32());
					player_entity->set_position(position);
					auto rotation = Vector3(packet.f32(), packet.f32(), packet.f32());
					player_entity->set_rotation(position);
					auto current_animation_str = packet.str();

					// Distribute updates to other clients
					auto update_packet = new BufWriter();
					update_packet->u8(ServerPacket::UPDATE_PLAYER_MOVEMENT);
					update_packet->u32(sender_id); // player ID
					update_packet->str(phase_scene_str); // phase scene
					update_packet->f32(position.x); // position
					update_packet->f32(position.y);
					update_packet->f32(position.z);
					update_packet->f32(rotation.x); // rotation
					update_packet->f32(rotation.y);
					update_packet->f32(rotation.z);
					update_packet->str(current_animation_str); // current animation
					send_to_all(update_packet);
					break;
				}
				case ClientPacket::ACTION_TAKE_DAMAGE: {
					auto health = packet.u32();

					auto health_packet = new BufWriter();
					health_packet->u8(ServerPacket::UPDATE_PLAYER_HEALTH);
					health_packet->u32(sender_id);
					health_packet->u32(health);
					send_to_all(health_packet);
					break;
				}
				case ClientPacket::ACTION_DROP: {
					break;
				}
				case ClientPacket::ACTION_GRAB: {
					break;
				}
				case ClientPacket::ACTION_USE: {
					break;
				}
				case ClientPacket::ACTION_CHAT_MESSAGE: {
					auto message = (string) packet.str();
					auto chat_packet = new BufWriter();
					chat_packet->u8(ServerPacket::CHAT_MESSAGE);
					chat_packet->i32(sender_id);
					chat_packet->str(message);
					send_to_all(chat_packet);
					delete chat_packet;
					break;
				}
			}
		}
	}

	// Query new world state & run game loop
	for (auto &[id, entity_info] : _entities) {
		auto update_packet = new BufWriter();
		update_packet->u8(ServerPacket::UPDATE_ENTITY);
		update_packet->u32(id);
		auto tracked_properties = entity_info->get_tracked_properties();
		update_packet->u16(tracked_properties.size());
		for (auto property : tracked_properties) {
			if (entity_info->tracked_property_changed(property)) {
				auto property_utf8 = property.utf8().get_data();
				update_packet->str(property_utf8);
				auto value = entity_info->get_property_value(property);
				auto temp_buffer = PackedByteArray();
				temp_buffer.encode_var(0, value);
				update_packet->str(temp_buffer.ptr(), temp_buffer.size());
			}
		}
		send_to_all(update_packet);
	}

	_game_time += delta;
	_tick_count++;
}

void encode_property()
{


}

void Server::_on_peer_connected(int id)
{
	// Init client datastructures
	auto client_socket = _socket_server->get_peer(id);
	EntityPlayer* client_body = nullptr;
	auto load_error = load_scene_strict<EntityPlayer>("res://scenes/entity_player.tscn", &client_body);
	if (load_error != godot::Error::OK || client_body == nullptr) {
		UtilityFunctions::print("Failed to connect peer: Player entity scene was invalid");
		client_socket->close(4000, "Internal server error");
		return;
	}
	auto client_data = new ClientData(client_socket, client_body);
	_clients[id] = client_data;

	// Send initial game state info to client
	auto game_info_packet = new BufWriter();
	game_info_packet->u8(ServerPacket::GAME_INFO);
	// TODO: Implement players waiting
	game_info_packet->u32((uint32_t) _clients.size()); // players waiting
	game_info_packet->u32(id); // (their) player id
	send(id, game_info_packet);
	delete game_info_packet;

	// Send initial playerlist to client
	auto player_info_packet = new BufWriter();
	player_info_packet->u8(ServerPacket::PLAYERS_INFO);
	player_info_packet->u16(_clients.size());
	for (auto &[player_id, client]  : _clients) {
		// TODO: Implement chat name
		player_info_packet->i32(player_id); // player_id
		player_info_packet->i32(0); // user_int_id
		string chat_name = "anon";
		player_info_packet->str(chat_name); // chat_name
	}
	send(id, player_info_packet);
	delete player_info_packet;

	// Alert all other clients of the new player
	auto new_player_info_packet = new BufWriter();
	new_player_info_packet->u8(ServerPacket::PLAYERS_INFO);
	new_player_info_packet->u16(1);
	// TODO: Implement chat name
	new_player_info_packet->i32(id);
	new_player_info_packet->i32(0); // user_int_id
	string chat_name = "anon";
	new_player_info_packet->str(chat_name); // chat_name
	send_to_all(new_player_info_packet);
	delete new_player_info_packet;
}

void Server::_on_peer_disconnected(int id)
{
	delete _clients[id];
	_clients.erase(id);
}

godot::Error Server::register_phase_scene(String identifier, String path)
{
	auto control_scene_resource = _resource_loader->load(path);
	if (!control_scene_resource.is_valid() || !control_scene_resource->is_class("PackedScene")) {
		UtilityFunctions::printerr("Failed to load scene scene: file not found");
		return godot::Error::ERR_FILE_NOT_FOUND;
	}

	Ref<PackedScene> packed_scene = control_scene_resource;
	if (!packed_scene.is_valid() || !packed_scene->can_instantiate()) {
		UtilityFunctions::printerr("Failed to load scene: resource was invalid");
		return godot::Error::ERR_INVALID_DATA;
	}

	auto scene_instance = packed_scene->instantiate();
	add_child(scene_instance);
	_phase_scenes.insert(identifier, scene_instance);
	return godot::Error::OK;
}

EntityInfo* Server::register_entity(Node* entity, String parent_scene)
{
	// Create new entity locally
	int max_id = 0;
	for (auto entity : _entities) {
		auto id = entity.key;
		if (id > max_id) {
			max_id = id;
		}
	}
	int new_id = max_id + 1;

	auto info = new EntityInfo(new_id, entity, parent_scene);
	_entities.insert(new_id, info);

	// Distribute to all clients
	auto entity_info_packet = new BufWriter();
	entity_info_packet->u8(ServerPacket::ENTITIES_INFO); // packet code
	entity_info_packet->u32(new_id); // entity id
	auto parent_scene_str = parent_scene.utf8().get_data();
	entity_info_packet->str(parent_scene_str); // parent_scene
	// Write node data (properties, children)
	write_entity_data(entity, entity_info_packet); // entity data

	send_to_all(entity_info_packet);
	delete entity_info_packet;
	return info;
}

void Server::run_console_loop()
{
	ReplIO repl;
	while (interface(repl,
		func(pack(this, &Server::set_phase), "set_phase", "Set the game phase to the specified stage",
			param("name", "String name of phase")),
		func(pack(this, &Server::create_entity), "create_entity", "Create a new entity of specified type",
			param("type", "Node type name of entity to be created")),
		func(pack(this, &Server::delete_entity), "delete_entity", "Delete a specific entity",
			param("id", "Id of entity to be deleted")),
		func(pack(this, &Server::update_entity), "update_entity", "Update a property of a specific entity",
			param("id", "Id of entity to be created"),
			param("property", "Name of property to be modified"),
			param("value", "New value to set of specified property")),
		func(pack(this, &Server::list_players), "list_players", "List all connected players"),
		func(pack(this, &Server::kill_player), "kill_player", "Kill a specific player",
			param("id", "Id of player to be killed")),
		func(pack(this, &Server::kick_player), "kick_player", "Disconnect a specific player",
			param("id", "Id of player to be disconnected")),
		func(pack(this, &Server::tp_player), "tp_player", "Teleport a specific players",
			param("scene", "String name of scene in which to teleport player"),
			param("x", "X-coordinate of new location"),
			param("y", "Y-coordinate of new location"),
			param("z", "Z-coordinate of new location")),
		func(pack(this, &Server::announce), "announce", "Send a message to all connected players",
			param("message", "Chat message to be broadcast"))));

	get_tree()->get_root()->call_deferred(
		"propagate_notification", NOTIFICATION_WM_CLOSE_REQUEST);
	get_tree()->quit(0);
}

void Server::set_phase(string name)
{
	auto phase_parts = String(name.c_str()).split(":");
	auto phase_scene = phase_parts.size() > 0 ? phase_parts[0] : "";
	auto phase_event = phase_parts.size() > 1 ? phase_parts[1] : "";
	if (phase_scene == "") {
		UtilityFunctions::print(
			"Couldn't set phase to {0}: Invalid phase scene name", String(name.c_str()));
		return;
	}

	// Run phase event on server replication scenes
	if (phase_scene == "roof") {
		auto roof_scene = (Roof*) _phase_scenes["roof"];
		roof_scene->call_deferred("_server_run_phase_event", phase_event);
	}
	else if (phase_scene == "end") {
		auto end_scene = (End*) _phase_scenes["end"];
		end_scene->call_deferred("_server_run_phase_event", phase_event);
	}

	// Run phase event on clients
	auto phase_packet = new BufWriter();
	phase_packet->u8(ServerPacket::SET_PHASE);
	phase_packet->str(name);
	send_to_all(phase_packet);
	delete phase_packet;
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
	auto player_list = String("Showing {0} players:\n")
		.format(Array::make(_clients.size()));
	for (auto &[player_id, client] : _clients) {
		auto client_entity = client->get_entity();
		player_list += String("{0}: { chat_name: {1}, health: {2}, position: {3}, rotation: {4} }")
			.format(Array::make(player_id, client_entity->get_chat_name(),
				client_entity->get_health(), client_entity->get_position(),
				client_entity->get_rotation()));
		player_list += "\n";
	}
	UtilityFunctions::print(player_list);
}

void Server::kill_player(int id)
{
	if (!_clients.has(id)) {
		UtilityFunctions::print("Coud not kill player ", id, ": player not found");
		return;
	}
	auto health_packet = new BufWriter();
	health_packet->u8(ServerPacket::UPDATE_PLAYER_HEALTH);
	health_packet->u32(id); // player_id
	health_packet->u32(0); // health
	send_to_all(health_packet);
}

void Server::kick_player(int id)
{
	if (!_clients.has(id)) {
		UtilityFunctions::print("Could not kick player ", id, ": player not found");
		return;
	}

	_clients[id]->get_socket()->close(4000, "You were kicked from the server");
}

void Server::tp_player(string scene, int x, int y, int z)
{
}

void Server::announce(string message)
{
	auto chat_packet = new BufWriter();
	chat_packet->u8(ServerPacket::CHAT_MESSAGE);
	chat_packet->i32(0); // player_id
	chat_packet->str(message);
	send_to_all(chat_packet);
	delete chat_packet;
}

void Server::send_to_all(BufWriter* packet)
{
	send_to_all(packet->data(), packet->size());
}

void Server::send_to_all(const char* data, size_t size)
{
	auto packed_data = PackedByteArray();
	packed_data.resize(size);
	memcpy(packed_data.ptrw(), data, size);

	for (auto &[id, client] : _clients) {
		client->get_socket()->put_packet(packed_data);
	}
}

void Server::send(int id, BufWriter* packet)
{
	send(id, packet->data(), packet->size());
}

void Server::send(int id, const char* data, size_t size)
{
	auto packed_data = PackedByteArray();
	packed_data.resize(size);
	memcpy(packed_data.ptrw(), data, size);
	_clients[id]->get_socket()->put_packet(packed_data);
}
