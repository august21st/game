#include <godot_cpp/classes/web_socket_multiplayer_peer.hpp>
#include <godot_cpp/classes/random_number_generator.hpp>
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
#include <godot_cpp/classes/resource_saver.hpp>
#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/camera3d.hpp>
#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/classes/time.hpp>
#include <godot_cpp/classes/mutex.hpp>
#include <godot_cpp/classes/reg_ex.hpp>
#include <godot_cpp/classes/os.hpp>
#include <godot_cpp/classes/global_constants.hpp>
#include <godot_cpp/classes/item_list.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>
#include <godot_cpp/variant/vector3.hpp>
#include <godot_cpp/classes/expression.hpp>
#include <godot_cpp/classes/v_scroll_bar.hpp>
#include <dataproto_cpp/dataproto.hpp>
#include <commandIO.hpp>

#include "server.hpp"
#include "client_data.hpp"
#include "entity_item_base.hpp"
#include "godot_cpp/variant/callable.hpp"
#include "network_shared.hpp"
#include "node_shared.hpp"
#include "entity_player.hpp"
#include "entity_info.hpp"
#include "packet_info.hpp"
#include "server_camera.hpp"
#include "roof.hpp"
#include "end.hpp"

using namespace commandIO;
using namespace dataproto;
using namespace godot;
using namespace std;
using namespace NetworkShared;
using namespace NodeShared;

#define INTERVAL_SECONDS(count) (_tick_count % (count * TPS) == 0)

Server::Server() :
	_engine(nullptr),
	_socket_server(nullptr),
	_display_server(nullptr),
	_resource_loader(nullptr),
	_time(nullptr),
	_server_scenes_container(nullptr),
	_incoming_packets_list(nullptr),
	_incoming_packets_filter(nullptr),
	_incoming_packets_filter_clear(nullptr),
	_incoming_packets_enabled(nullptr),
	_outgoing_packets_list(nullptr),
	_outgoing_packets_filter(nullptr),
	_outgoing_packets_filter_clear(nullptr),
	_outgoing_packets_enabled(nullptr),
	_entities_lock(nullptr),
	_bbcode_regex(nullptr)
{
}

Server::~Server()
{
	if (_socket_server.is_valid()) {
		_socket_server->disconnect("peer_connected", Callable(this, "_on_peer_connected"));
		_socket_server->disconnect("peer_disconnected", Callable(this, "_on_peer_disconnected"));
		_socket_server->close();
	}

	// Clean up dynamically allocated resources
	if (_entities_lock) {
		memdelete(_entities_lock);
	}

	// Clean up client data
	for (auto &[id, client] : _clients) {
		memdelete(client);
	}

	// Clean up entity info
	for (auto &[id, entity_info] : _entities) {
		memdelete(entity_info);
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

	// GUI
	ClassDB::bind_method(D_METHOD("set_incoming_packets_logging"),
		&Server::set_incoming_packets_logging);
	ClassDB::bind_method(D_METHOD("get_incoming_packets_logging"),
		&Server::get_incoming_packets_logging);
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "incoming_packets_logging"),
		"set_incoming_packets_logging", "get_incoming_packets_logging");
	ClassDB::bind_method(D_METHOD("_on_incoming_packets_filter_text_submitted", "new_text"),
		&Server::_on_incoming_packets_filter_text_submitted);
	ClassDB::bind_method(D_METHOD("_on_incoming_packets_filter_clear_pressed"),
		&Server::_on_incoming_packets_filter_clear_pressed);
	ClassDB::bind_method(D_METHOD("_on_incoming_packets_enabled_pressed"),
		&Server::_on_incoming_packets_enabled_pressed);
	ClassDB::bind_method(D_METHOD("set_outgoing_packets_logging"),
		&Server::set_outgoing_packets_logging);
	ClassDB::bind_method(D_METHOD("get_outgoing_packets_logging"),
		&Server::get_outgoing_packets_logging);
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "outgoing_packets_logging"),
		"set_outgoing_packets_logging", "get_outgoing_packets_logging");
	ClassDB::bind_method(D_METHOD("_on_outgoing_packets_filter_text_submitted", "new_text"),
		&Server::_on_outgoing_packets_filter_text_submitted);
	ClassDB::bind_method(D_METHOD("_on_outgoing_packets_filter_clear_pressed"),
		&Server::_on_outgoing_packets_filter_clear_pressed);
	ClassDB::bind_method(D_METHOD("_on_outgoing_packets_enabled_pressed"),
		&Server::_on_outgoing_packets_enabled_pressed);

	ADD_SIGNAL(MethodInfo("packet_received",
		PropertyInfo(Variant::INT, "sender_id"),
		PropertyInfo(Variant::PACKED_BYTE_ARRAY, "packed_packet")));
}

void Server::_ready()
{
	UtilityFunctions::print("Starting as server...");
	_engine = Engine::get_singleton();
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
	_server_scenes = { };
	_phase_scenes = { };
	auto intro_err = register_phase_scene("rplace/intro", "res://scenes/rplace/intro.tscn");
	if (intro_err != godot::Error::OK) {
		return;
	}
	auto roof_err = register_phase_scene("rplace/roof", "res://scenes/rplace/roof.tscn");
	if (roof_err != godot::Error::OK) {
		return;
	}
	auto end_err = register_phase_scene("rplace/end", "res://scenes/rplace/end.tscn");
	if (end_err != godot::Error::OK) {
		return;
	}
	UtilityFunctions::print("Sucessfully loaded phase scenes");

	// Initialise CLI
	_console_thread = Ref<Thread>();
	_console_thread.instantiate();
	_console_thread->start(Callable(this, "run_console_loop"));

	// Initialise GUI
	// TODO: Only initialise in non-headless graphical mode
	_display_server = DisplayServer::get_singleton();
	_server_scenes_container = get_node<Node3D>("%ServerScenesContainer");
	_incoming_packets_logging = false;
	_incoming_packets = {};
	_incoming_packets_list = get_node<ItemList>("%IncomingPacketsList");
	_incoming_packets_filter = get_node<LineEdit>("%IncomingPacketsFilter");
	_incoming_packets_filter->connect("text_submitted", Callable(this, "_on_incoming_packets_filter_text_submitted"));
	_incoming_packets_filter_clear = get_node<Button>("%IncomingPacketsFilterClear");
	_incoming_packets_filter_clear->connect("pressed", Callable(this, "_on_incoming_packets_filter_clear_pressed"));
	_incoming_packets_enabled = get_node<CheckButton>("%IncomingPacketsEnabled");
	_incoming_packets_enabled->connect("pressed", Callable(this, "_on_incoming_packets_enabled_pressed"));
	_outgoing_packets_logging = false;
	_outgoing_packets = {};
	_outgoing_packets_list = get_node<ItemList>("%OutgoingPacketsList");
	_outgoing_packets_filter = get_node<LineEdit>("%OutgoingPacketsFilter");
	_outgoing_packets_filter->connect("text_submitted", Callable(this, "_on_outgoing_packets_filter_text_submitted"));
	_outgoing_packets_filter_clear = get_node<Button>("%OutgoingPacketsFilterClear");
	_outgoing_packets_filter_clear->connect("pressed", Callable(this, "_on_outgoing_packets_filter_clear_pressed"));
	_outgoing_packets_enabled = get_node<CheckButton>("%OutgoingPacketsEnabled");
	_outgoing_packets_enabled->connect("pressed", Callable(this, "_on_outgoing_packets_enabled_pressed"));

	// Initialise properties
	_clients = HashMap<int, ClientData*>();
	_authenticated_clients = HashMap<int, ClientData*>();
	_entities = HashMap<int, EntityInfo*>();
	_entities_lock = memnew(Mutex());
	_entities_head = 0;
	_start_time = _time->get_unix_time_from_system();
	_game_time = 0.0L;
	_tick_count = 0;
	_bbcode_regex = memnew(RegEx());
	_bbcode_regex->compile("\\[.*?\\]");
	_engine->set_physics_ticks_per_second(TPS);
	set_process(true);
	set_physics_process(true);

	// Check for fuzz testing
	auto os = OS::get_singleton();
	_fuzz_testing = os->get_cmdline_args().has("--fuzz");
	if (_fuzz_testing) {
		UtilityFunctions::print("Starting in fuzz testing mode...");
		initialize_fuzzing();
	}
}

void Server::set_incoming_packets_logging(bool value)
{
	_incoming_packets_logging = value;
}

bool Server::get_incoming_packets_logging()
{
	return _incoming_packets_logging;
}

void Server::_on_incoming_packets_filter_text_submitted(String new_text)
{
	eval_packet_filter(new_text, _incoming_packets, _incoming_packets_list);
}

void Server::_on_incoming_packets_enabled_pressed()
{
	set_incoming_packets_logging(!get_incoming_packets_logging());
}

void Server::_on_incoming_packets_filter_clear_pressed()
{
	eval_packet_filter("true", _incoming_packets, _incoming_packets_list);
}

void Server::set_outgoing_packets_logging(bool value)
{
	_outgoing_packets_logging = value;
}

bool Server::get_outgoing_packets_logging()
{
	return _outgoing_packets_logging;
}

void Server::_on_outgoing_packets_filter_text_submitted(String new_text)
{
	eval_packet_filter(new_text, _outgoing_packets, _outgoing_packets_list);
}

void Server::_on_outgoing_packets_enabled_pressed()
{
	set_outgoing_packets_logging(!get_outgoing_packets_logging());
}

void Server::_on_outgoing_packets_filter_clear_pressed()
{
	eval_packet_filter("true", _outgoing_packets, _outgoing_packets_list);
}

void Server::eval_packet_filter(String filter, List<PacketInfo*> infos, ItemList* info_list)
{
	auto expression = Ref<Expression>();
	expression.instantiate();

	// Parse the expression and define variable names
	auto variables = Array::make("code", "code_name", "from", "to", "time");
	auto parse_error = expression->parse(filter, variables);
	if (parse_error != OK) {
		UtilityFunctions::printerr("Failed to evaluate packet filter: ", expression->get_error_text());
		return;
	}

	for (auto info : infos) {
		auto values = Array::make(info->code, info->code_name, info->from, info->to, info->time);
		auto result = expression->execute(values);
		if (expression->has_execute_failed()) {
			UtilityFunctions::printerr("Failed to evaluate packet filter: Expression execution failed");
			continue;
		}

		if (result.get_type() != Variant::BOOL) {
			UtilityFunctions::printerr("Failed to evaluate packet filter: Expression didn't produce bool");
			continue;
		}
		bool matches = result.booleanize();
		info_list->set_item_disabled(info->item_index, !matches);
	}
}

void Server::add_incoming_packet_info(ClientPacket code, ClientData* from)
{
	auto packet_info = memnew(PacketInfo());
	packet_info->code = to_uint8(code);
	packet_info->code_name = NetworkShared::to_string(code);
	packet_info->from = String("{0} ({1})")
		.format(Array::make(from->get_entity()->get_id(), from->get_entity()->get_chat_name()));
	auto time_string = _time->get_time_string_from_system();
	packet_info->time = time_string;
	auto info_string = String("code: {0} ({1}) | from: {2} | time: {3}")
		.format(Array::make(packet_info->code, packet_info->code_name, packet_info->from, packet_info->time));
	auto item_i = _incoming_packets_list->add_item(info_string, nullptr, false);
	packet_info->item_index = item_i;
	_incoming_packets.push_back(packet_info);
	packets_list_autoscroll(_incoming_packets_list->get_v_scroll_bar());
}

void Server::add_outgoing_packet_info(ServerPacket code, ClientData* to)
{
	auto packet_info = memnew(PacketInfo());
	packet_info->code = to_uint8(code);
	packet_info->code_name = NetworkShared::to_string(code);
	packet_info->to = to == nullptr
		? String("all")
		: String("{0} ({1})")
			.format(Array::make(to->get_entity()->get_id(), to->get_entity()->get_chat_name()));
	auto time_string = _time->get_time_string_from_system();
	packet_info->time = time_string;
	auto info_string = String("code: {0} ({1}) | to: {2} | time: {3}")
		.format(Array::make(packet_info->code, packet_info->code_name, packet_info->to, packet_info->time));
	auto item_i = _outgoing_packets_list->add_item(info_string, nullptr, false);
	packet_info->item_index = item_i;
	_outgoing_packets.push_back(packet_info);
	packets_list_autoscroll(_outgoing_packets_list->get_v_scroll_bar());
}

void Server::packets_list_autoscroll(VScrollBar* scroll_bar)
{
	auto scroll_max = scroll_bar->get_max();
	auto scroll_current = scroll_bar->get_value() + scroll_bar->get_size().y;
	if (scroll_max - scroll_current <= 64) {
		scroll_bar->set_value(scroll_max);
	}
}

void Server::_physics_process(double delta)
{
	// Testing
	if (_fuzz_testing) {
		process_fuzzing(delta);
	}

	// Handle new WS updates
	if (_socket_server.is_valid()) {
		_socket_server->poll();
		while (_socket_server->get_available_packet_count()) {
			auto sender_id = _socket_server->get_packet_peer();
			if (!_clients.has(sender_id)) {
				UtilityFunctions::printerr("Could not handle data from sender ", sender_id, ": not found in clients map");
				return;
			}
			auto sender = _clients[sender_id];
			auto packed_packet = _socket_server->get_packet();
			auto packet = BufReader((char*) packed_packet.ptr(), packed_packet.size());
			auto code = static_cast<ClientPacket>(packet.u8());

			// Server GUI packet logging
			if (get_incoming_packets_logging()) {
				add_incoming_packet_info(code, sender);
			}

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
					send_to_other_players(sender_id, new_player_info_packet);
					break;
				}
				case ClientPacket::ACTION_CHAT_MESSAGE: {
					auto message_str = (string) packet.str();
					auto message = String(message_str.c_str());
					// Remove potential injected BBCode TODO: Add chat markdown support
					message = _bbcode_regex->sub(message, "", true);

					auto chat_packet = BufWriter();
					chat_packet.u8(to_uint8(ServerPacket::CHAT_MESSAGE));
					chat_packet.i32(sender_id);
					chat_packet.str(message.utf8().get_data());
					send_to_players(chat_packet);
					break;
				}
				default: {
					break;
				}
			}

			emit_signal("packet_received", sender_id, packed_packet);
		}
	}

	// Query new world state & run game loop
	_entities_lock->lock();
	// TODO: A node similar to godot's MultiplayerSynchroniser could be used to
	// track changes in entities and send updates only when necessary such that not
	// all objects have to derive from Entity
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
				write_variant(value, update_packet);
			}
		}
		send_to_players(update_packet);
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

// TODO: Construction of the client body is too early, especially for unauthenticated servelist players, move to after AUTHENTICATE
void Server::_on_peer_connected(int id)
{
	// Init client datastructures
	auto client_socket = _socket_server->get_peer(id);
	EntityPlayer* client_body = nullptr;
	auto load_error = load_scene_strict<EntityPlayer>("res://scenes/entity_player.tscn", &client_body);
	if (load_error != godot::Error::OK || client_body == nullptr) {
		UtilityFunctions::printerr("Failed to connect peer: Player entity scene was invalid");
		client_socket->close(4000, "Internal server error");
		return;
	}

	add_child(client_body);
	client_body->set_id(id);
	auto chat_name = String("anon");
	client_body->set_chat_name(chat_name);

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
	if (!_phase_scenes.has(parent_scene)) {
		UtilityFunctions::printerr("Can't create entity ", node_path,
			"in scene", parent_scene, ": phase scene doesn't exist");
		return nullptr;
	}

	_phase_scenes[parent_scene]->add_child(entity_node);
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
	write_scene_data(node_path, entity_info_packet); // entity data

	send_to_players(entity_info_packet);
	return info;
}

// TODO: Use entity& ref
EntityInfo* Server::register_entity(Node* entity, String parent_scene)
{
	if (entity == nullptr) {
		UtilityFunctions::printerr("Couldn't register entity: Entity was null");
		return nullptr;
	}
	if (!_phase_scenes.has(parent_scene)) {
		UtilityFunctions::printerr("Couldn't register entity: Specified parent scene not found");
		return nullptr;
	}
	_entities_lock->lock();
	// TODO: Investigate optimisations
	// Prevent double entity registrations
	for (auto &[id, info] : _entities) {
		if (entity == info->get_entity()) {
			// Ignore double registration
			UtilityFunctions::print("Ignoring double registration for entity", entity-> get_class());
			_entities_lock->unlock();
			return nullptr;
		}
	}

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
	write_scene_data(entity, entity_info_packet); // entity data

	send_to_players(entity_info_packet);
	return info;
}

EntityInfo* Server::get_entity(int id)
{
	_entities_lock->lock();
	if (!_entities.has(id)) {
		_entities_lock->unlock();
		return nullptr;
	}
	auto entity_info = _entities[id];
	_entities_lock->unlock();
	return entity_info;
}

void Server::run_console_loop()
{
	ReplIO repl;
	while (interface(repl,
		func(pack(this, &Server::repl_set_phase), "set_phase", "Set the game phase to the specified stage",
			param("name", "String name of phase"),
			param("unload_previous", "Unload previous phase")),
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
		func(pack(this, &Server::tp_player), "tp_player", "Teleport a specific players",
			param("scene", "String name of scene in which to teleport player"),
			param("x", "X-coordinate of new location"),
			param("y", "Y-coordinate of new location"),
			param("z", "Z-coordinate of new location")),
		func(pack(this, &Server::kick_client), "kick_client", "Disconnect a specific client",
			param("id", "Id of client to be disconnected")),
		func(pack(this, &Server::repl_announce), "announce", "Send a message to all connected players",
			param("message", "Chat message to be broadcast"))));

	// Exit
	get_tree()->get_root()->call_deferred("propagate_notification", NOTIFICATION_WM_CLOSE_REQUEST);
	get_tree()->quit(0);
}

void Server::repl_set_phase(string name, string unload_previous)
{
	bool unload_previous_bool = unload_previous == "true";
	call_deferred("set_phase", String(name.c_str()), unload_previous_bool);
}

void Server::set_phase(String name, bool unload_previous)
{
	auto phase_parts = name.split(":");
	auto phase_scene = phase_parts.size() > 0 ? phase_parts[0] : "";
	auto phase_event = phase_parts.size() > 1 ? phase_parts[1] : "";
	if (phase_scene.is_empty() || !_phase_scenes.has(phase_scene)) {
		UtilityFunctions::printerr("Couldn't set phase to ", name, ": Phase scene not found");
		return;
	}

	// ServerScenesContainer (Node3D)
	// ├── Phase1_Viewport (Viewport)
	// │   ├── ServerCamera
	// │   └── [SceneInstance] (Loaded Phase Scene)
	// ├── Phase2_Viewport (Viewport)
	// │   ├── ServerCamera
	// │   └── [SceneInstance] (Loaded Phase Scene)
	// └── ...


	_current_phase_scene = phase_scene;
	_current_phase_event = phase_event;

	for (auto &[identifier, scene_node] : _phase_scenes) {
		auto identifier_parts = identifier.split(":");
		auto identifier_scene = identifier_parts.size() > 0 ? identifier_parts[0] : "";

		// If current scene
		if (identifier_scene == phase_scene) {
			if (!scene_node->is_inside_tree()) {
				// Create isolated viewport for this scene
				Viewport* viewport = memnew(Viewport);

				// Create server camera for this scene
				ServerCamera* camera = memnew(ServerCamera);
				camera->set_current(true);
				camera->set_transform(Transform3D(Basis(), Vector3(0, 0, 0)));
				viewport->add_child(camera);

				// Add scene to viewport
				viewport->add_child(scene_node);

				// Add viewport to server scenes & scene
				_server_scenes_container->add_child(viewport);
				_server_scenes.insert(phase_scene, viewport);
			}
		}
		else if (unload_previous) {
			if (scene_node->is_inside_tree()) {
				Viewport* viewport = _server_scenes.get(phase_scene);
				if (viewport && viewport->is_inside_tree()) {
					// Orphan scene
					viewport->remove_child(scene_node);

					// Delete viewport
					_server_scenes_container->remove_child(viewport);
					_server_scenes.erase(phase_scene);
					viewport->queue_free();
				}
			}
		}
	}

	// Handle phase-specific logic
	if (phase_scene == "rplace/roof") {
		auto container = _phase_scenes["rplace/roof"];
		auto viewport = Object::cast_to<Viewport>(container->get_child(0));
		auto roof_scene = Object::cast_to<Roof>(viewport->get_child(0));
		roof_scene->call_deferred("run_phase_event", phase_event);
	}
	else if (phase_scene == "rplace/end") {
		auto container = _phase_scenes["rplace/end"];
		auto viewport = Object::cast_to<Viewport>(container->get_child(0));
		auto end_scene = Object::cast_to<End>(viewport->get_child(0));
		end_scene->call_deferred("run_phase_event", phase_event);
	}

	// Notify clients
	auto phase_packet = BufWriter();
	phase_packet.u8(to_uint8(ServerPacket::SET_PHASE));
	auto name_str = name.utf8().get_data();
	phase_packet.str(name_str);
	send_to_all(phase_packet);

	// Move all players to new scene
	for (auto &[id, player_info] : _authenticated_clients) {
		player_info->get_entity()->respawn(name, Vector3(0, 0, 0));
	}
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
	auto player_list = String("Showing {0} players (websocket clients: {1}):\n")
		.format(Array::make(_authenticated_clients.size(), _clients.size()));
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
	if (!_authenticated_clients.has(id)) {
		UtilityFunctions::printerr("Coud not kill player ", id, ": player not found");
		return;
	}
	_authenticated_clients[id]->get_entity()->die();
}

void Server::kick_client(int id)
{
	if (!_clients.has(id)) {
		UtilityFunctions::printerr("Could not kick player ", id, ": player not found");
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
	send_to_players(chat_packet);
}

void Server::send_to_other_players(int exclude_id, const BufWriter& packet)
{
	send_to_other_players(exclude_id, packet.data(), packet.size());
}

void Server::send_to_other_players(int exclude_id, const char* data, size_t size)
{
	auto packed_data = PackedByteArray();
	packed_data.resize(size);
	memcpy(packed_data.ptrw(), data, size);

	for (auto &[id, client] : _clients) {
		if (id != exclude_id) {
			send(id, packed_data);
		}
	}
}

void Server::send_to_all(const BufWriter& packet)
{
	send_to_all(packet.data(), packet.size());

	// Server GUI packet logging
	if (get_outgoing_packets_logging() && packet.size() > 0) {
		// Sneak peak code
		auto code = static_cast<ServerPacket>(packet.data()[0]);
		add_outgoing_packet_info(code);
	}
}

void Server::send_to_all(const char* data, size_t size)
{
	auto packed_data = PackedByteArray();
	packed_data.resize(size);
	memcpy(packed_data.ptrw(), data, size);

	for (auto &[id, client] : _clients) {
		send(id, packed_data);
	}
}

void Server::send_to_players(const BufWriter& packet)
{
	send_to_players(packet.data(), packet.size());

	// Server GUI packet logging
	if (get_outgoing_packets_logging() && packet.size() > 0) {
		// Sneak peak code
		auto code = static_cast<ServerPacket>(packet.data()[0]);
		add_outgoing_packet_info(code);
	}
}

void Server::send_to_players(const char* data, size_t size)
{
	auto packed_data = PackedByteArray();
	packed_data.resize(size);
	memcpy(packed_data.ptrw(), data, size);

	for (auto &[id, client] : _authenticated_clients) {
		send(id, packed_data);
	}
}

void Server::send(int id, const BufWriter& packet)
{
	send(id, packet.data(), packet.size());

	// Server GUI packet logging
	if (get_outgoing_packets_logging() && packet.size() > 0) {
		// Sneak peak code
		auto code = static_cast<ServerPacket>(packet.data()[0]);
		add_outgoing_packet_info(code, _clients[id]);
	}
}

void Server::send(int id, const char* data, size_t size)
{
	auto packed_data = PackedByteArray();
	packed_data.resize(size);
	memcpy(packed_data.ptrw(), data, size);

	send(id, packed_data);
}

void Server::send(int id, PackedByteArray packed_data)
{
	if (!_clients.has(id)) {
		UtilityFunctions::printerr("Couldn't send packet: client with id ", id, " not found");
		return;
	}

	auto client = _clients[id];
	if (client == nullptr || !client->get_socket().is_valid()) {
		UtilityFunctions::printerr("Couldn't send packet: Invalid client or socket for id ", id);
		return;
	}

	client->get_socket()->put_packet(packed_data);
}

String Server::get_current_phase_scene()
{
	return _current_phase_scene;
}

String Server::get_current_phase_event()
{
	return _current_phase_event;
}

bool Server::has_phase_scene(String name)
{
	return _phase_scenes.has(name);
}

Node* Server::get_phase_scene(String name)
{
	if (!_phase_scenes.has(name)) {
		return nullptr;
	}

	return _phase_scenes.get(name);
}

bool Server::is_client()
{
	return false;
}

bool Server::is_server()
{
	return true;
}

void Server::initialize_fuzzing()
{
	_rng.instantiate();
	_rng->randomize();

	_fuzz_socket = Ref<WebSocketPeer>();
	_fuzz_socket.instantiate();
	connect_fuzz_socket();
}

void Server::connect_fuzz_socket()
{
	if (_fuzz_socket->get_ready_state() != WebSocketPeer::STATE_CLOSED) {
		return;
	}

	auto server_url = String("ws://localhost:{0}").format( Array::make(SERVER_PORT));
	auto connect_error = _fuzz_socket->connect_to_url(server_url);

	// TODO: Add retry
	if (connect_error == godot::Error::OK) {
		UtilityFunctions::print("Fuzz testing: Loopback socket to ", server_url, " connected successfully");
	}
	else {
		UtilityFunctions::printerr("Couldn't connect fuzz loopback socket: Received error ", connect_error);
	}
}

void Server::process_fuzzing(double delta)
{
	_fuzz_socket->poll();
	auto state = _fuzz_socket->get_ready_state();

	if (state == WebSocketPeer::STATE_OPEN) {
		while (_fuzz_socket->get_available_packet_count()) {
			auto packet = _fuzz_socket->get_packet();
			UtilityFunctions::print("Fuzz testing: Packet with code ", packet.decode_u8(0), " received");
		}

		// TODO: Add a scene timer, conduct scene fuzzing with random phase loads and changes.
		// TODO: Add an entity timer, conduct entity fuzzy with random entity spawning, manipulation.
		// TODO: Add client stressing to fuzzer where client can connect to server that will blast it with random packets.
		// TODO: Intelligent fuzzing, such as move packets in valid phase scenes, etc to cover more code. Consider moving
		// TODO: packet fuzzer to client side so multiple clients can connect & fuzz (Server stressing)
		// TODO: Such that client fuzz stresses server & server fuzz stresses clients

		auto fuzz_packet = generate_fuzz_packet();
		_fuzz_socket->send(fuzz_packet);
		maybe_disconnect_fuzz();
	}
	else if (state == WebSocketPeer::STATE_CLOSING) {
		UtilityFunctions::print("Fuzz testing: Socket ready state transitioning to closed");
	}
	else if (state == WebSocketPeer::STATE_CLOSED) {
		// Attempt to reconnect if disconnected
		if (_rng->randf() < 0.1) {
			connect_fuzz_socket();
		}
	}
}

PackedByteArray Server::generate_fuzz_packet()
{
	BufWriter packet;

	// Randomly choose a packet type
	ClientPacket packet_type = static_cast<ClientPacket>(
		_rng->randi_range(0, static_cast<int>(ClientPacket::ACTION_CHAT_MESSAGE)));
	packet.u8(to_uint8(packet_type));

	for (auto i = 0; i < _rng->randi_range(0, 512); i++) {
		packet.u8(_rng->randi_range(0, 255));
	}

	PackedByteArray result;
	result.resize(packet.size());
	memcpy(result.ptrw(), packet.data(), packet.size());
	return result;
}

void Server::maybe_disconnect_fuzz()
{
	// 1% chance to disconnect per packet
	if (_rng->randf() < 0.01) {
		UtilityFunctions::print("Fuzz testing: Random disconnect");
		_fuzz_socket->close();
	}
}

String Server::generate_random_string(int length)
{
	String result;
	for (int i = 0; i < length; i++) {
		result += String::chr(_rng->randi());
	}
	return result;
}

Vector3 Server::generate_random_vector3()
{
	return Vector3(
		_rng->randf(),
		_rng->randf(),
		_rng->randf()
	);
}
