#include <cstdint>
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
#include <godot_cpp/classes/camera3d.hpp>
#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/classes/time.hpp>
#include <godot_cpp/classes/mutex.hpp>
#include <dataproto_cpp/dataproto.hpp>
#include <commandIO.hpp>

#include "server.hpp"
#include "client_data.hpp"
#include "entity_item_base.hpp"
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

#define INTERVAL_SECONDS(count) (_tick_count % (count * TPS) == 0)

Server::Server() : _socket_server(nullptr), _server_camera(nullptr)
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
	ClassDB::bind_method(D_METHOD("set_phase", "name"), &Server::set_phase);
	ClassDB::bind_method(D_METHOD("create_entity", "node_path", "parent_scene"), &Server::create_entity);
	ClassDB::bind_method(D_METHOD("register_entity", "entity", "parent_scene"), &Server::register_entity);
	ClassDB::bind_method(D_METHOD("list_players"), &Server::list_players);
	ClassDB::bind_method(D_METHOD("list_entities"), &Server::list_entities);
}

void Server::_ready()
{
	_engine = Engine::get_singleton();
	if (!is_server()) {
		set_physics_process(false);
		return;
	}
	UtilityFunctions::print("Starting as server...");

	_time = Time::get_singleton();

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

	// Initialise GUI (if in graphical mode)
	_display_server = DisplayServer::get_singleton();
	if (_display_server->get_name() != "headless") {
		_server_camera = get_node<Camera3D>("%ServerCamera");
		_server_scene = get_node<Node3D>("%ServerScene");
	}

	// Initialise properties
	_clients = HashMap<int, ClientData*>();
	_authenticated_clients = HashMap<int, ClientData*>();
	_entities = HashMap<int, EntityInfo*>();
	_entities_lock = memnew(Mutex());
	_entities_head = 0;
	_start_time = _time->get_unix_time_from_system();
	_game_time = 0.0L;
	_tick_count = 0;
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
			auto packet = BufReader((char*) data.ptr(), data.size());
			auto code = static_cast<ClientPacket>(packet.u8());
			switch (code) {
				case ClientPacket::AUTHENTICATE: {
					// Client has choosen us in the server list, and will now
					// be subscribed to game events
					sender->authenticate();
					_authenticated_clients[sender_id] = sender;

					// Send initial game state info to client
					auto game_info_packet = BufWriter();
					game_info_packet.u8(to_uint8(ServerPacket::GAME_INFO));
					game_info_packet.u32(sender_id); // (their) player id
					send(sender_id, game_info_packet);

					// Send initial playerlist to client
					auto player_info_packet = BufWriter();
					player_info_packet.u8(to_uint8(ServerPacket::PLAYERS_INFO));
					player_info_packet.u16(_clients.size()); // player_count
					for (auto &[player_id, client] : _clients) {
						write_player_info(player_id, client, player_info_packet); // player_info
					}
					send(sender_id, player_info_packet);

					// Alert all other clients of the new player
					auto new_player_info_packet = BufWriter();
					new_player_info_packet.u8(to_uint8(ServerPacket::PLAYERS_INFO));
					new_player_info_packet.u16(1); // player_count
					write_player_info(sender_id, sender, new_player_info_packet); // player_info
					send_to_others(sender_id, new_player_info_packet);
					break;
				}
				case ClientPacket::SET_CHAT_NAME: {
					auto chat_name_str = (string) packet.str();
					auto chat_name = String(chat_name_str.c_str());
					sender->get_entity()->set_chat_name(chat_name);

					break;
				}
				case ClientPacket::SET_MODEL_VARIANT: {
					bool valid = false;
					// TODO: Fix me!
					/*for (auto i = 0; i < size(MODEL_VARIANTS); i++) {
						if (MODEL_VARIANTS[i] == colour) {
							valid = true;
						}
						}*/
					if (valid) {

					}
					break;
				}
				case ClientPacket::UPDATE_MOVEMENT: {
					auto phase_scene_str = (string) packet.str();
					auto phase_scene = String(phase_scene_str.c_str());
					if (!_phase_scenes.has(phase_scene)) {
						UtilityFunctions::print("Couldn't update movement for client ",
							sender_id, ": phase scene doesn't exist");
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
					auto position = read_vector3(packet); // position
					player_entity->set_global_position(position);
					auto velocity = read_vector3(packet); // velocity
					auto rotation = read_vector3(packet); // rotation
					player_entity->set_global_rotation(rotation);
					auto current_animation_str = packet.str();

					// Distribute updates to other clients
					auto update_packet = BufWriter();
					update_packet.u8(to_uint8(ServerPacket::UPDATE_PLAYER_MOVEMENT));
					update_packet.u32(sender_id); // player ID
					update_packet.str(phase_scene_str); // phase scene
					write_vector3(update_packet, position); // position
					write_vector3(update_packet, velocity); // velocity
					write_vector3(update_packet, rotation); // rotation
					update_packet.str(current_animation_str); // current animation
					send_to_others(sender_id, update_packet);
					break;
				}
				case ClientPacket::ACTION_TAKE_DAMAGE: {
					auto health = packet.u32();
					// TODO: Fully implement

					auto health_packet = BufWriter();
					health_packet.u8(to_uint8(ServerPacket::UPDATE_PLAYER_HEALTH));
					health_packet.u32(sender_id);
					health_packet.u32(health);
					send_to_authenticated(health_packet);
					break;
				}
				case ClientPacket::ACTION_DIE: {
					break;
				}
				case ClientPacket::ACTION_RESPAWN: {
					break;
				}
				case ClientPacket::ACTION_DROP: {
					break;
				}
				case ClientPacket::ACTION_GRAB: {
					_entities_lock->lock();
					auto item_id = packet.u32();
					if (!_entities.has(item_id)) {
						_entities_lock->unlock();
						break;
					}
					auto entity_info = _entities[item_id];
					auto item_entity = Object::cast_to<EntityItemBase>(entity_info->get_entity());
					if (item_entity == nullptr) {
						// What the sigma
						_entities_lock->unlock();
						break;
					}
					_entities_lock->unlock();
					orphan_node(item_entity);
					auto player_body = sender->get_entity();
					player_body->add_child(item_entity);
					auto inventory = player_body->get_inventory();
					inventory->push_back(item_entity);
					player_body->set_inventory_current(inventory->size() - 1);

					auto grab_packet = BufWriter();
					grab_packet.u8(to_uint8(ServerPacket::GRAB));
					grab_packet.u32(sender_id);
					grab_packet.u32(item_id);
					send_to_authenticated(grab_packet);
					break;
				}
				case ClientPacket::ACTION_SWITCH: {
					break;
				}
				case ClientPacket::ACTION_USE: {
					break;
				}
				case ClientPacket::ACTION_CHAT_MESSAGE: {
					auto message = (string) packet.str();
					auto chat_packet = BufWriter();
					chat_packet.u8(to_uint8(ServerPacket::CHAT_MESSAGE));
					chat_packet.i32(sender_id);
					chat_packet.str(message);
					send_to_authenticated(chat_packet);
					break;
				}
			}
		}
	}

	// Query new world state & run game loop
	_entities_lock->lock();
	for (auto &[id, entity_info] : _entities) {
		auto update_packet = BufWriter();
		update_packet.u8(to_uint8(ServerPacket::UPDATE_ENTITY));
		update_packet.u32(id);
		auto tracked_properties = entity_info->get_tracked_properties();
		update_packet.flint(tracked_properties.size());
		for (auto property : tracked_properties) {
			if (entity_info->tracked_property_changed(property)) {
				auto property_utf8 = property.utf8().get_data();
				update_packet.str(property_utf8);
				auto value = entity_info->get_property_value(property);
				write_compressed_variant(value, update_packet);
			}
		}
		send_to_authenticated(update_packet);
	}
	_entities_lock->unlock();

	if (INTERVAL_SECONDS(1)) {
		distribute_server_info();
	}
	_game_time += delta;
	_tick_count++;
}

int Server::hash_string(String value)
{
	int hash = 0;
	for (auto i = 0; i < value.length(); i++) {
		hash = hash * 31 + int(value[i]) & 0xffffffff;
	}
	return hash;
}

void Server::write_player_info(int id, ClientData* client, BufWriter& buffer)
{
	buffer.i32(id); // player_id
	auto chat_name = client->get_entity()->get_chat_name();
	auto chat_name_str = chat_name.utf8().get_data();
	buffer.str(chat_name_str); // chat_name
	auto model_variant = client->get_entity()->get_model_variant();
	auto model_variant_str = model_variant.utf8().get_data();
	buffer.str(model_variant_str); // model_variant
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
	auto chat_name = String("anon");
	auto client_data = memnew(ClientData(client_socket, client_body));
	_clients[id] = client_data;

	distribute_server_info();
}

void Server::_on_peer_disconnected(int id)
{
	memdelete(_clients[id]);
	_clients.erase(id);
	if (_authenticated_clients.has(id)) {
		_authenticated_clients.erase(id);
	}
}

void Server::distribute_server_info()
{
	// Server list details
	auto server_info_packet = BufWriter();
	server_info_packet.u8(to_uint8(ServerPacket::SERVER_INFO));
	auto duration_s = _time->get_unix_time_from_system() - _start_time;
	server_info_packet.u32(duration_s); // duration_s
	// TODO: Use authenticated clients size
	server_info_packet.u32(_clients.size()); // player_count
	server_info_packet.u32(PLAYER_LIMIT); // player_limit
	// TODO: Send as phase:event
	auto phase_str = _current_phase_scene.utf8().get_data();
	server_info_packet.str(phase_str); // phase
	send_to_all(server_info_packet);
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
	_phase_scenes.insert(identifier, scene_instance);
	return godot::Error::OK;
}

uint Server::next_entity_id()
{
	_entities_lock->lock();
	_entities_head++;
	while (_entities.has(_entities_head)) {
		_entities_head++;
	}
	_entities_lock->unlock();
	return _entities_head;
}

void Server::repl_create_entity(string node_path, string parent_scene)
{
	call_deferred("create_entity", String(node_path.c_str()), String(parent_scene.c_str()));
}

// Entities created with create_entity needn't be registed, this method
// will more efficiently instance an entity from a path, and instruct
// clients to do so as  well, opposed to encoding the entire node inline
EntityInfo* Server::create_entity(String node_path, String parent_scene)
{
	_entities_lock->lock();
	auto new_id = next_entity_id();

	Node* entity_node;
	auto load_error = load_scene(node_path, &entity_node);
	if (load_error != godot::Error::OK || entity_node == nullptr) {
		return nullptr;
	}
	auto info = memnew(EntityInfo(new_id, entity_node, parent_scene));
	_entities.insert(new_id, info);
	_entities_lock->unlock();

	// Distribute to all clients
	auto entity_info_packet = BufWriter();
	entity_info_packet.u8(to_uint8(ServerPacket::ENTITIES_INFO)); // packet code
	entity_info_packet.u16(1); //entity count
	entity_info_packet.u32(new_id); // entity id
	auto parent_scene_str = parent_scene.utf8().get_data();
	entity_info_packet.str(parent_scene_str); // parent_scene
	// Write node data (node scene) - ObjectType::FILESYSTEM_NODE
	write_entity_data(node_path, entity_info_packet); // entity data

	send_to_authenticated(entity_info_packet);
	return info;
}

EntityInfo* Server::register_entity(Node* entity, String parent_scene)
{
	_entities_lock->lock();
	auto new_id = next_entity_id();
	auto info = memnew(EntityInfo(new_id, entity, parent_scene));
	_entities.insert(new_id, info);
	_entities_lock->unlock();

	// Distribute to all clients
	auto entity_info_packet = BufWriter();
	entity_info_packet.u8(to_uint8(ServerPacket::ENTITIES_INFO)); // packet code
	entity_info_packet.u16(1); //entity count
	entity_info_packet.u32(new_id); // entity id
	auto parent_scene_str = parent_scene.utf8().get_data();
	entity_info_packet.str(parent_scene_str); // parent_scene
	// Write node data (properties, children) - ObjectType::INLINE_NODE
	write_entity_data(entity, entity_info_packet); // entity data

	send_to_authenticated(entity_info_packet);
	return info;
}

void Server::run_console_loop()
{
	ReplIO repl;
	while (interface(repl,
		func(pack(this, &Server::repl_set_phase), "set_phase", "Set the game phase to the specified stage",
			param("name", "String name of phase")),
		func(pack(this, &Server::repl_create_entity), "create_entity", "Create a new entity of specified type",
			param("node_path", "Path to scene containing entity node"),
			param("parent_scene", "Identifier name of phase scene containing entity")),
		/*func(pack(this, &Server::delete_entity), "delete_entity", "Delete a specific entity",
			param("id", "Id of entity to be deleted")),
		func(pack(this, &Server::repl_update_entity), "update_entity", "Update a property of a specific entity",
			param("id", "Id of entity to be created"),
			param("property", "Name of property to be modified"),
			param("value", "New value to set of specified property")),*/
		func(pack(this, &Server::repl_list_entities), "list_entities", "List all entities"),
		func(pack(this, &Server::repl_list_players), "list_players", "List all authenticated players"),
		func(pack(this, &Server::kill_player), "kill_player", "Kill a specific player",
			param("id", "Id of player to be killed")),
		func(pack(this, &Server::kick_player), "kick_player", "Disconnect a specific player",
			param("id", "Id of player to be disconnected")),
		func(pack(this, &Server::tp_player), "tp_player", "Teleport a specific players",
			param("scene", "String name of scene in which to teleport player"),
			param("x", "X-coordinate of new location"),
			param("y", "Y-coordinate of new location"),
			param("z", "Z-coordinate of new location")),
		func(pack(this, &Server::repl_announce), "announce", "Send a message to all connected players",
			param("message", "Chat message to be broadcast"))));

	// Exit
	get_tree()->get_root()->call_deferred("propagate_notification", NOTIFICATION_WM_CLOSE_REQUEST);
	get_tree()->quit(0);
}

void Server::repl_set_phase(string name)
{
	call_deferred("set_phase", String(name.c_str()));
}

void Server::set_phase(String name)
{
	auto phase_parts = name.split(":");
	auto phase_scene = phase_parts.size() > 0 ? phase_parts[0] : "";
	auto phase_event = phase_parts.size() > 1 ? phase_parts[1] : "";
	if (phase_scene == "") {
		UtilityFunctions::print("Couldn't set phase to ", name, ": Invalid phase scene name");
		return;
	}

	// Update current phase scene and event
	_current_phase_scene = phase_scene;
	_current_phase_event = phase_event;

	// If multiple scenes are present, they will overlap physics and cause chaos -
	// only one scene can be in tree at a time to prevent interference
	for (auto &[identifier, scene_node] : _phase_scenes) {
		auto identifier_parts = identifier.split(":");
		auto identifier_scene = identifier_parts.size() > 0 ? identifier_parts[0] : "";
		if (identifier_scene == phase_scene && scene_node->get_parent() != _server_scene) {
			// current scene
			_server_scene->add_child(scene_node);
			if (_server_camera != nullptr) {
				_server_camera->set_position(Vector3(0, 0, 0));
				_server_camera->set_current(true);
			}
		}
		else {
			// Other scene
			_server_scene->remove_child(scene_node);
		}
	}

	// Run phase event on server replication scenes
	if (phase_scene == "roof") {
		auto roof_scene = (Roof*) _phase_scenes["roof"];
		roof_scene->call_deferred("server_run_phase_event", phase_event);
	}
	else if (phase_scene == "end") {
		auto end_scene = (End*) _phase_scenes["end"];
		end_scene->call_deferred("server_run_phase_event", phase_event);
	}

	// Run phase event on clients
	auto phase_packet = BufWriter();
	phase_packet.u8(to_uint8(ServerPacket::SET_PHASE));
	auto name_str = name.utf8().get_data();
	phase_packet.str(name_str);
	send_to_all(phase_packet);
}

void Server::delete_entity(int id)
{
	_entities.erase(id);
	// TODO: Distribute to clients
}

void Server::repl_update_entity(int id, string property, string value)
{
	// TODO: Implement this!
}

void Server::repl_list_players()
{
	call_deferred("list_players");
}

// Prints player list with main player properties in a JSON-like manner
void Server::list_players()
{
	auto player_list = String("Showing {0} players:\n")
		.format(Array::make(_authenticated_clients.size()));
	for (auto &[player_id, client] : _authenticated_clients) {
		auto client_entity = client->get_entity();
		player_list += String("{0}: { chat_name: {1}, health: {2}, position: {3}, rotation: {4} }")
			.format(Array::make(player_id, client_entity->get_chat_name(),
				client_entity->get_health(), client_entity->get_position(),
				client_entity->get_rotation()));
		player_list += "\n";
	}
	UtilityFunctions::print(player_list);
}

void Server::repl_list_entities()
{
	call_deferred("list_entities");
}

// Prints entity list with tracked entity properties and metadata in a JSON-like manner
void Server::list_entities()
{
	auto entity_list = String("Showing {0} entities:\n")
		.format(Array::make(_entities.size()));
	for (auto &[entity_id, entity_info] : _entities) {
		auto entity = entity_info->get_entity();
		if (entity == nullptr) {
			entity_list += String("{0}: null").format(Array::make(entity_id));
			continue;
		}

		entity_list += String("{0}: { parent_scene: {1}")
			.format(Array::make(entity_id, entity_info->get_parent_scene()));
		for (auto property : entity_info->get_tracked_properties()) {
			entity_list += String(", {0}: {1}")
				.format(Array::make(property, entity->get(property)));
		}
		entity_list += " }\n";
	}
	UtilityFunctions::print(entity_list);
}

void Server::kill_player(int id)
{
	if (!_clients.has(id)) {
		UtilityFunctions::print("Coud not kill player ", id, ": player not found");
		return;
	}
	auto health_packet = BufWriter();
	health_packet.u8(to_uint8(ServerPacket::UPDATE_PLAYER_HEALTH));
	health_packet.u32(id); // player_id
	health_packet.u32(0); // health
	send_to_authenticated(health_packet);
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

void Server::repl_announce(string message)
{
	auto chat_packet = BufWriter();
	chat_packet.u8(to_uint8(ServerPacket::CHAT_MESSAGE));
	chat_packet.i32(0); // player_id
	chat_packet.str(message);
	send_to_authenticated(chat_packet);
}

void Server::send_to_others(int exclude_id, const BufWriter& packet)
{
	send_to_others(exclude_id, packet.data(), packet.size());
}

void Server::send_to_others(int exclude_id, const char* data, size_t size)
{
	auto packed_data = PackedByteArray();
	packed_data.resize(size);
	memcpy(packed_data.ptrw(), data, size);

	for (auto &[id, client] : _clients) {
		if (id != exclude_id) {
			client->get_socket()->put_packet(packed_data);
		}
	}
}

void Server::send_to_all(const BufWriter& packet)
{
	send_to_all(packet.data(), packet.size());
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

void Server::send_to_authenticated(const BufWriter& packet)
{
	send_to_authenticated(packet.data(), packet.size());
}

void Server::send_to_authenticated(const char* data, size_t size)
{
	auto packed_data = PackedByteArray();
	packed_data.resize(size);
	memcpy(packed_data.ptrw(), data, size);

	for (auto &[id, client] : _authenticated_clients) {
		client->get_socket()->put_packet(packed_data);
	}
}

void Server::send(int id, const BufWriter& packet)
{
	send(id, packet.data(), packet.size());
}

void Server::send(int id, const char* data, size_t size)
{
	auto packed_data = PackedByteArray();
	packed_data.resize(size);
	memcpy(packed_data.ptrw(), data, size);
	_clients[id]->get_socket()->put_packet(packed_data);
}

String Server::get_current_phase_scene()
{
	return _current_phase_scene;
}

String Server::get_current_phase_event()
{
	return _current_phase_event;
}
