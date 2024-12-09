#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/panel.hpp>
#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/classes/input_event.hpp>
#include <godot_cpp/classes/h_slider.hpp>
#include <godot_cpp/classes/button.hpp>
#include <godot_cpp/variant/packed_byte_array.hpp>
#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/classes/packed_scene.hpp>
#include <godot_cpp/classes/audio_server.hpp>
#include <godot_cpp/classes/option_button.hpp>
#include <godot_cpp/classes/input.hpp>
#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/classes/rich_text_label.hpp>
#include <godot_cpp/templates/hash_map.hpp>
#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/templates/cowdata.hpp>
#include <godot_cpp/classes/translation_server.hpp>
#include <godot_cpp/classes/java_script_bridge.hpp>
#include <godot_cpp/classes/window.hpp>
#include <godot_cpp/templates/list.hpp>
#include <godot_cpp/classes/display_server.hpp>
#include <godot_cpp/classes/timer.hpp>
#include <godot_cpp/core/math.hpp>
#include <godot_cpp/core/memory.hpp>
#include <godot_cpp/core/error_macros.hpp>
#include <godot_cpp/core/property_info.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/string.hpp>

#include "client.hpp"
#include "roof.hpp"
#include "end.hpp"
#include "player_body.hpp"
#include "entity_player_base.hpp"
#include "entity_player.hpp"
#include "network_shared.hpp"
#include "node_shared.hpp"

using namespace godot;
using namespace dataproto;
using namespace NetworkShared;
using namespace NodeShared;

Client::Client() : _socket(Ref<WebSocketPeer>()), _socket_closed(true),
	_player_body(nullptr), _stats_label(nullptr)
{
}

Client::~Client()
{
    // Clean up dynamically allocated resources
    for (auto &[identifier, scene_info] : _phase_scenes) {
        memdelete(scene_info);
    }
    _phase_scenes.clear();

    // Clear entity and player maps
    _entities.clear();
    _players.clear();

    // Unreference the WebSocketPeer
    if (_socket.is_valid()) {
        _socket.unref();
    }

    // Clean up player body if it exists
    if (_player_body) {
        memdelete(_player_body);
        _player_body = nullptr;
    }
}

void Client::_bind_methods()
{
	ClassDB::bind_method(D_METHOD("_on_pause_button_pressed"),
		&Client::_on_pause_button_pressed);
	ClassDB::bind_method(D_METHOD("_on_volume_slider_drag_started"),
		&Client::_on_volume_slider_drag_started);
	ClassDB::bind_method(D_METHOD("_on_volume_slider_drag_ended"),
		&Client::_on_volume_slider_drag_ended);
	ClassDB::bind_method(D_METHOD("_on_volume_slider_value_changed"),
		&Client::_on_volume_slider_value_changed);
	ClassDB::bind_method(D_METHOD("_on_graphics_options_item_selected", "index"),
		&Client::_on_graphics_options_item_selected);
	ClassDB::bind_method(D_METHOD("_on_back_button_pressed"),
		&Client::_on_back_button_pressed);
	ClassDB::bind_method(D_METHOD("_on_quit_button_pressed"),
		&Client::_on_quit_button_pressed);
	ClassDB::bind_method(D_METHOD("_on_current_scene_ready"),
		&Client::_on_current_scene_ready);
	ClassDB::bind_method(D_METHOD("_on_setup_preset_button_pressed"),
		&Client::_on_setup_preset_button_pressed);
	ClassDB::bind_method(D_METHOD("_on_setup_confirm_button_pressed"),
		&Client::_on_setup_confirm_button_pressed);
	ClassDB::bind_method(D_METHOD("_on_setup_language_options_item_selected", "index"),
		&Client::_on_setup_language_options_item_selected);
	ClassDB::bind_method(D_METHOD("_on_language_options_item_selected", "index"),
		&Client::_on_language_options_item_selected);
	ClassDB::bind_method(D_METHOD("_on_player_entity_ready", "id", "chat_name", "model_variant"),
		&Client::_on_player_entity_ready);
	ClassDB::bind_method(D_METHOD("_on_alert_close_button_pressed"),
		&Client::_on_alert_close_button_pressed);

	ADD_SIGNAL(MethodInfo("packet_received",
		PropertyInfo(Variant::PACKED_BYTE_ARRAY, "packed_packet")));
	ADD_SIGNAL(MethodInfo("graphics_quality_changed",
		PropertyInfo(Variant::INT, "level")));
	ADD_SIGNAL(MethodInfo("volume_changed",
		PropertyInfo(Variant::INT, "volume_ratio")));
	ADD_SIGNAL(MethodInfo("current_scene_ready"));
	ADD_SIGNAL(MethodInfo("stats_changed",
		PropertyInfo(Variant::INT, "level")));
}

void Client::_ready()
{
	_os = OS::get_singleton();
	_engine = Engine::get_singleton();
	_client_gui = get_node<Control>("%ClientGui");

	_performance = Performance::get_singleton();
	_translation_server = TranslationServer::get_singleton();

	_resource_loader = ResourceLoader::get_singleton();
	_audio_server = AudioServer::get_singleton();
	_player_input = Input::get_singleton();

	_client_scene = get_node<Node3D>("%ClientScene");
	_stats_label = get_node<Label>("%StatsLabel");
	set_stats_enabled(false);

	_pause_panel = get_node<Panel>("%PausePanel");
	set_paused(false);

	_pause_button = get_node<Button>("%PauseButton");
	_pause_button->connect("pressed", Callable(this, "_on_pause_button_pressed"));

	_volume_label = get_node<RichTextLabel>("%VolumeLabel");
	_volume_label->set_visible(false);

	_volume_slider = get_node<HSlider>("%VolumeSlider");
	_volume_slider->connect("drag_started", Callable(this, "_on_volume_slider_drag_started"));
	_volume_slider->connect("drag_ended", Callable(this, "_on_volume_slider_drag_ended"));
	_volume_slider->connect("value_changed", Callable(this, "_on_volume_slider_value_changed"));

	_graphics_options = get_node<OptionButton>("%GraphicsOptions");
	_graphics_options->connect("item_selected", Callable(this, "_on_graphics_options_item_selected"));
	_current_graphics_level = 1;

	_language_options = get_node<OptionButton>("%LanguageOptions");
	_language_options->connect("item_selected", Callable(this, "_on_language_options_item_selected"));
	_translation_server->set_locale("en");

	_back_button = get_node<Button>("%BackButton");
	_back_button->connect("pressed", Callable(this, "_on_back_button_pressed"));
	_close_button = get_node<Button>("%CloseButton");
	_close_button->connect("pressed", Callable(this, "_on_back_button_pressed"));

	_quit_button = get_node<Button>("%QuitButton");
	_quit_button->connect("pressed", Callable(this, "_on_quit_button_pressed"));

	_setup_panel = get_node<Panel>("%SetupPanel");
	_setup_panel->set_visible(true);

	_select_preset_label = get_node<RichTextLabel>("%SelectPreesetLabel");
	_select_preset_label->set_visible(false);
	_mobile_presets_button = get_node<Button>("%MobilePresetsButton");
	_mobile_presets_button->connect("pressed", Callable(this, "_on_setup_preset_button_pressed"));
	_pc_presets_button = get_node<Button>("%PcPresetsButton");
	_pc_presets_button->connect("pressed", Callable(this, "_on_setup_preset_button_pressed"));

	_alert_panel = get_node<Panel>("%AlertPanel");
	_alert_panel->set_visible(false);
	_alert_label = get_node<Label>("%AlertLabel");
	_alert_close_button = get_node<Button>("%AlertCloseButton");

	// Try predict platform from feature flags
	if (_os->has_feature("web_android") || _os->has_feature("web_ios")
		|| _os->has_feature("android") || _os->has_feature("ios")) {
		_mobile_presets_button->set_pressed(true);
	}
	else if (_os->has_feature("web_linuxbsd") || _os->has_feature("web_windows") || _os->has_feature("web_macos")
		|| _os->has_feature("linuxbsd") || _os->has_feature("windows") || _os->has_feature("macos"))  {
		_pc_presets_button->set_pressed(true);
	}

	_setup_language_options = get_node<OptionButton>("%SetupLanguageOptions");
	_setup_language_options->connect("item_selected", Callable(this, "_on_setup_language_options_item_selected"));

	_setup_confirm_button = get_node<Button>("%SetupConfirmButton");
	_setup_confirm_button->connect("pressed", Callable(this, "_on_setup_confirm_button_pressed"));

	_entities = { };
	_players = { };
	_phase_scenes = { };
	register_phase_scene("loading_screen", "res://scenes/loading_screen.tscn");
	register_phase_scene("intro", "res://scenes/intro.tscn");
	register_phase_scene("roof", "res://scenes/roof.tscn");
	register_phase_scene("end", "res://scenes/end.tscn");

	UtilityFunctions::print("Starting as client...");
	PlayerBody* player_body_scene = nullptr;
	auto load_error = load_scene_strict<PlayerBody>("res://scenes/player_body.tscn", &player_body_scene);
	if (load_error != Error::OK || player_body_scene == nullptr) {
		UtilityFunctions::print("Couldn't start client, player body scene failed to load");
		return;
	}
	_player_body = player_body_scene;

	// Hand over to loading screen, which will organise selecting a server
	set_process(false);
	set_fail_function([](const void* reason_ptr)
	{
		auto reason = String(static_cast<const char*>(reason_ptr));
		UtilityFunctions::printerr(String("dataproto error: {0}").format(Array::make(reason)));
	});
	change_scene("loading_screen");
}

// Will add phase scene to _phase_scenes and start preloading it
Error Client::register_phase_scene(String identifier, String path)
{
	auto preload_error = _resource_loader->load_threaded_request(path);
	auto scene_info = memnew(PhaseSceneInfo({ .scene_path = path, .load_status = PhaseSceneLoadStatus::REQUESTED }));
	_phase_scenes.insert(identifier, scene_info);
	return preload_error;
}

Node* Client::get_current_scene()
{
	auto scene_node = _client_scene->get_child(0);
	return scene_node;
}

template<typename T>
T* Client::get_current_scene_strict()
{
	auto scene_node = get_current_scene();
	if (scene_node->get_class() != T::get_class_static()) {
		return nullptr;
	}

	return (T*) scene_node;
}

double Client::round_decimal(double value, int places)
{
	return Math::round(value*Math::pow(10.0, places)) / Math::pow(10.0, places);
}

// Player has selected this server (socket) from the loading screen, start
// the client using this socket instance
void Client::start_with_socket(Ref<WebSocketPeer> socket)
{
	_socket = socket;
	if (_socket.is_valid()) {
		_socket->reference();
	}

	_socket_closed = false;

	// Accept query params chat name and model variant on web
	if (_os->has_feature("JavaScript")) {
		_js_bridge = JavaScriptBridge::get_singleton();
		auto chat_name_variant = _js_bridge->eval("new URL(window.location.href).searchParams.get(\"chatName\")");
		if (chat_name_variant.get_type() != Variant::STRING) {
			auto chat_name = (String) chat_name_variant;
			auto name_packet = BufWriter();
			name_packet.u8(to_uint8(ClientPacket::SET_CHAT_NAME));
			auto chat_name_str = chat_name.utf8().get_data();
			name_packet.str(chat_name_str);
			send(name_packet);
		}

		auto colour_variant = _js_bridge->eval("new URL(window.location.href).searchParams.get(\"colour\")");
		if (colour_variant.get_type() != Variant::STRING) {
			auto colour = (String) colour_variant;
			auto model_packet = BufWriter();
			model_packet.u8(to_uint8(ClientPacket::SET_MODEL_VARIANT));
			auto colour_str = colour.utf8().get_data();
			model_packet.str(colour_str);
			send(model_packet);
		}
	}

	UtilityFunctions::print("Starting game with socket ", socket->get_requested_url());
	set_process(true);

	auto auth_packet = BufWriter();
	auth_packet.u8(to_uint8(ClientPacket::AUTHENTICATE));
	send(auth_packet);
}

Ref<WebSocketPeer> Client::get_socket()
{
	return _socket;
}

Error Client::send(const BufWriter& packet)
{
	send(packet.data(), packet.size());
	return Error::OK;
}

Error Client::send(const char* data, size_t size)
{
	if (!_socket.is_valid()) {
		return Error::ERR_UNAVAILABLE;
	}

	auto packed_data = PackedByteArray();
    packed_data.resize(size);
    memcpy(packed_data.ptrw(), data, size);
    _socket->put_packet(packed_data);
    return Error::OK;
}

List<PackedByteArray> Client::poll_next_packets()
{
	auto packets = List<PackedByteArray>();
	if (_socket_closed || !_socket.is_valid()) {
		return packets;
	}

	_socket->poll();
	auto state = _socket->get_ready_state();
	switch (state) {
		case WebSocketPeer::STATE_CONNECTING:
			break;
		case WebSocketPeer::STATE_OPEN:
			while (_socket->get_available_packet_count()) {
				auto packet = _socket->get_packet();
				packets.push_back(packet);
			}
			break;
		case WebSocketPeer::STATE_CLOSING:
			break;
		case WebSocketPeer::STATE_CLOSED:
			auto code = _socket->get_close_code();
			auto reason = _socket->get_close_reason();
			UtilityFunctions::print("Disconnected from client websocket: Code: ",
				code, "Reason: ", reason);
			_socket_closed = true;

			// Go back to loading screen. which will allow us to get connected to a new
			// server / back to the server socket, while appearing elegant to a client
			change_scene("loading_screen");
			auto alert_message = String("You have been disconnected from the server.\n"
				"Close code {0}, reason: '{1}'.\nPlease restart the game or reconnect via the server list.")
					.format(Array::make(code, reason));
			show_alert(alert_message);
			break;
	}

	return packets;
}

void Client::_input(const Ref<InputEvent> &event)
{
	auto mouse_mode = _player_input->get_mouse_mode();

	if (event->is_action_pressed("toggle_stats")) {
		set_stats_enabled(!_stats_enabled);
	}
	else if (event->is_action_pressed("toggle_pause") && mouse_mode != Input::MOUSE_MODE_CAPTURED) {
		set_paused(!_paused);
	}
}

void Client::_process(double delta)
{
	if (_stats_enabled) {
		update_stats();
	}

	if (!_socket_closed && _socket.is_valid()) {
		auto packed_packets = poll_next_packets();
		for (PackedByteArray packed_packet : packed_packets) {
			auto packet = BufReader((char*) packed_packet.ptr(), packed_packet.size());
			auto code = static_cast<ServerPacket>(packet.u8());
			switch (code) {
				case ServerPacket::GAME_INFO: {
					// Add ourselves into the generic player list
					auto player_id = packet.u32();
					_player_id = player_id;
					_players.insert(_player_id, _player_body);
					break;
				}
				case ServerPacket::PLAYERS_INFO: {
					auto player_count = packet.u16();
					for (auto i = 0; i < player_count; i++) {
						auto id = packet.u32();
						auto chat_name_str = (string) packet.str();
						auto chat_name = String(chat_name_str.c_str());
						auto model_variant_str = (string) packet.str();
						auto model_variant = String(model_variant_str.c_str());

						if (!_players.has(id)) {
							EntityPlayer* player_entity = nullptr;
							auto load_error = load_scene_strict<EntityPlayer>("res://scenes/entity_player.tscn", &player_entity);
							if (load_error != Error::OK || player_entity == nullptr) {
								UtilityFunctions::print("Failed to insert entity player: entity player scene failed to load.");
								return;
							}
							player_entity->connect("ready", Callable(this, "_on_player_entity_ready")
								.bind(id, chat_name, model_variant));
							_players.insert(id, player_entity);
						}
						else {
							auto player_entity = _players[id];
							player_entity->set_chat_name(chat_name);
							player_entity->set_model_variant(model_variant);
						}
					}
					break;
				}
				case ServerPacket::UPDATE_PLAYER_MOVEMENT: {
					auto id = packet.u32();
					if (!_players.has(id)) {
						if (id != _player_id) {
							UtilityFunctions::printerr("Could not update player ",
								id, ": player entity not found");
						}
						break;
					}
					auto phase_scene_str = (string) packet.str();
					auto phase_scene = String(phase_scene_str.c_str());
					if (phase_scene != _current_phase_scene) {
						// Player is not in our world, ignore it
						break;
					}

					auto player_entity = _players[id];
					auto position = read_vector3(packet); // position
					auto velocity = read_vector3(packet); // velocity
					auto rotation = read_vector3(packet); // rotation
					auto current_animation_str = (string) packet.str();
					auto current_animation = String(current_animation_str.c_str());
					auto current_scene_node = _client_scene->get_child(0);
					auto player_parent = player_entity->get_parent();
					if (player_parent != current_scene_node) {
						orphan_node(player_entity);
						current_scene_node->add_child(player_entity);
					}

					player_entity->set_global_position(position);
					player_entity->set_global_rotation(rotation);
					break;
				}
				case ServerPacket::UPDATE_PLAYER_HEALTH: {
					auto player_id = packet.u32();
					if (!_players.has(player_id)) {
						if (player_id != _player_id) {
							UtilityFunctions::printerr("Could not update health for player ",
								player_id, ": player entity not found");
						}
						break;
					}

					auto player_entity = _players[player_id];
					auto health = packet.u32();
					player_entity->set_health(health);
					break;
				}
				case ServerPacket::ENTITIES_INFO: {
					auto entity_count = packet.u16();
					for (auto i = 0; i < entity_count; i++) {
						auto id = packet.u32();
						auto parent_scene_str = (string) packet.str();
						auto parent_scene = String(parent_scene_str.c_str());

						auto entity_node = read_entity_data(packet);
						if (entity_node == nullptr) {
							UtilityFunctions::print("Failed to create entity ", id, ": failed to decode entity data");
							UtilityFunctions::print("Dumping entity info packet after reading ", i, " entities: BufReader state is corrupted");
							break;
						}
						// Register entity
						_entities.insert(id, entity_node);

						// If a scene is defined, spawn the entity in
						if (parent_scene == "roof" && _current_phase_scene == "roof") {
							auto roof_scene = get_current_scene_strict<Roof>();
							roof_scene->add_child(entity_node);
						}
						else if (parent_scene == "end" && _current_phase_scene == "end") {
							auto end_scene = get_current_scene_strict<End>();
							end_scene->add_child(entity_node);
						}
					}
					break;
				}
				case ServerPacket::RESPAWN: {
					auto player_id = packet.u32();
					auto position = read_vector3(packet);
					if (_players.has(player_id)) {
						auto player = _players[player_id];
						player->respawn(position);
					}
					else {
						UtilityFunctions::print("Could not respawn player ", player_id, ": player not found");
					}
					break;
				}
				case ServerPacket::UPDATE_ENTITY: {
					auto id = packet.u32();
					if (!_entities.has(id)) {
						UtilityFunctions::print("Couldn't update entity with id ", id, ": entity not found.");
						break;
					}
					auto entity = _entities[id];

					auto property_count = packet.flint();
					for (auto i = 0; i < property_count; i++) {
						auto property_str = (string) packet.str();
						auto property = String(property_str.c_str());
						auto value = read_compressed_variant(packet);
						entity->set(property, value);
					}
					break;
				}
				case ServerPacket::SET_PHASE: {
					auto phase_str = (string) packet.str();
					auto phase = String(phase_str.c_str());
					auto phase_parts = phase.split(":");
					auto phase_scene = phase_parts.size() > 0 ? phase_parts[0] : "";
					auto phase_event = phase_parts.size() > 1 ? phase_parts[1] : "";

					if (!_phase_scenes.has(phase_scene)) {
						UtilityFunctions::printerr("Could not set phase to ",
							phase, ": unknown phase");
						break;
					}

					if (phase_scene == "loading_screen") {
						change_scene("loading_screen");
					}
					if (phase_scene == "intro") {
						change_scene("intro");
					}
					else if (phase_scene == "roof") {
						if (_current_phase_scene != phase_scene) {
							change_scene("roof");
						}

						auto roof_scene = get_current_scene_strict<Roof>();
						// Remove player from wherever it was and hand it over
						if (_player_body->get_parent() != roof_scene) {
							orphan_node(_player_body);
							roof_scene->spawn_player(_player_body);
						}
						roof_scene->run_phase_event(phase_event);
					}
					else if (phase_scene == "end") {
						if (_current_phase_scene != phase_scene) {
							change_scene("end");
						}

						auto end_scene = get_current_scene_strict<End>();
						if (_player_body->get_parent() != end_scene) {
							orphan_node(_player_body);
							end_scene->spawn_player(_player_body);
						}
						end_scene->run_phase_event(phase_event);
					}
					else {
					}
					_current_phase_scene = phase_scene;
					_current_phase_event = phase_event;
					break;
				}
				case ServerPacket::GRAB: {
					auto player_id = packet.u32();
					if (!_players.has(player_id)) {
						UtilityFunctions::print("Could not handle item grab: player ", player_id, " not found");
					}
					auto entity_id = packet.u32();
					if (!_entities.has(entity_id)) {
						UtilityFunctions::print("Could not handle item grab: entity ", entity_id, " not found");
						break;
					}

					auto player = _players.get(player_id);
					UtilityFunctions::print("DEBUG: Grab");
					break;
				}
				default: {
					break;
				}
			}

			emit_signal("packet_received", packed_packet);
		}
	}
}

void Client::show_alert(String message)
{
	_alert_panel->set_visible(true);
	_alert_label->set_text(message);
}

void Client::_on_alert_close_button_pressed()
{
	_alert_panel->set_visible(false);
}

void Client::_on_player_entity_ready(int id, String chat_name, String model_variant)
{
	if (!_players.has(id)) {
		UtilityFunctions::print("Failed to initialise player ", id, " player was not in _players at time of _ready");
	}

	auto player_entity = _players[id];
	player_entity->set_chat_name(chat_name);
	player_entity->set_model_variant(model_variant);
}

void Client::set_stats_enabled(bool enable)
{
	_stats_enabled = enable;
	if (_stats_label != nullptr) {
		_stats_label->set_visible(enable);
	}
	emit_signal("stats_changed", enable);
}

void Client::update_stats()
{
	if (_stats_label == nullptr) {
		return;
	}

	auto fps = _engine->get_frames_per_second();
	auto physics_process = 1.0f / _performance->get_monitor(Performance::Monitor::TIME_PHYSICS_PROCESS);
	auto object_count = _performance->get_monitor(Performance::Monitor::OBJECT_COUNT);
	auto rendered_objects_count = _performance->get_monitor(Performance::Monitor::RENDER_TOTAL_OBJECTS_IN_FRAME);
	auto memory_static = _performance->get_monitor(Performance::Monitor::MEMORY_STATIC);
	auto memory_static_unit = "B";
	if (memory_static > 1'000'000) {
		memory_static_unit = "MB";
		memory_static = round_decimal(memory_static / 1'000'000, 2);
	}
	else if (memory_static > 1'000) {
		memory_static_unit = "KB";
		memory_static = Math::floor(memory_static / 1'000);
	}
	auto stats_string = String("Client:\nFPS: {0}\nPhysics process: {1}\nScene objects: "
		"{2}\nRendered objects: {3}\nStatic memory usage: {4}{5}\n")
		.format(Array::make(fps, physics_process, object_count, rendered_objects_count, memory_static, memory_static_unit));
	_stats_label->set_text(stats_string);
}

void Client::set_paused(bool pause)
{
	_paused = pause;
	if (_pause_panel != nullptr) {
		_pause_panel->set_visible(pause);
	}
}

void Client::_on_pause_button_pressed()
{
	set_paused(true);
}

void Client::_on_volume_slider_drag_started()
{
	_volume_label->set_visible(true);
}

void Client::_on_volume_slider_drag_ended(bool value_changed)
{
	_volume_label->set_visible(false);
}

Error Client::change_scene(String identifier)
{
	if (!_phase_scenes.has(identifier)) {
		return Error::ERR_DOES_NOT_EXIST;
	}

	auto scene_info = _phase_scenes.get(identifier);
	Node* scene_instance;
	if (scene_info->load_status == PhaseSceneLoadStatus::UNLOADED) {
		// Slow route, this will cause significant stutter
		auto load_error = load_scene(scene_info->scene_path, &scene_instance);
		if (load_error != Error::OK) {
			return load_error;
		}
	}
	else if (scene_info->load_status == PhaseSceneLoadStatus::REQUESTED) {
		// Hot route, continue from threaded request
		auto scene_resource = _resource_loader->load_threaded_get(scene_info->scene_path);
		if (!scene_resource.is_valid() || !scene_resource->is_class("PackedScene")) {
			UtilityFunctions::printerr("Failed to load scene from threaded request: file not found");
			return Error::ERR_FILE_NOT_FOUND;
		}

		Ref<PackedScene> packed_scene = scene_resource;
		if (!packed_scene.is_valid() || !packed_scene->can_instantiate()) {
			UtilityFunctions::printerr("Failed to load scene from threaded request: resource was invalid");
			return Error::ERR_INVALID_DATA;
		}

		scene_instance = packed_scene->instantiate();
	}
	else if (scene_info->load_status == PhaseSceneLoadStatus::LOADED) {
		scene_instance = scene_info->scene;
	}

	// Cache loaded scene to scene info
	if (scene_info->load_status != PhaseSceneLoadStatus::LOADED) {
		scene_info->load_status = PhaseSceneLoadStatus::LOADED;
		scene_info->scene = scene_instance;
	}

	while (_client_scene->get_child_count() > 0) {
		_client_scene->remove_child(_client_scene->get_child(0));
	}

	// Newly created scene won't be safe to touch until we have guarenteed it,
	// and all it's nodes have fully loaded
	scene_instance->connect("ready", Callable(this, "_on_current_scene_ready"));
	_client_scene->add_child(scene_instance);

	return Error::OK;
}

void Client::_on_current_scene_ready()
{
	auto scene_instance = get_current_scene();
	emit_signal("graphics_quality_changed", _current_graphics_level);
	emit_signal("volume_changed", _current_volume_ratio);
	emit_signal("current_scene_ready");
	emit_signal("stats_changed", _stats_enabled);
}

void Client::_on_volume_slider_value_changed(float value)
{
	auto volume_comment = _volume_comments[(int) Math::floor(value)];
	_volume_label->set_text(String("{0}% ({1})").format(Array::make(value, volume_comment)));
	_volume_label->set_size(_volume_label->get_minimum_size());
	auto slide_ratio = value / 100.0f;
	auto volume_label_size = _volume_label->get_size();
	_volume_label->set_scale(
		Vector2(0.4f, 0.4f) * slide_ratio + Vector2(0.8f, 0.8f));
	_volume_label->set_pivot_offset(Vector2(
		volume_label_size.x / 2, volume_label_size.y));
	auto slider_position = _volume_slider->get_global_rect().get_position();
	auto slider_size = _volume_slider->get_size();
	const float slider_x_padding = 8.0f;
	_volume_label->set_global_position(Vector2(
		(slider_position.x + slider_x_padding)
			+ (slide_ratio * (slider_size.x - slider_x_padding * 2.0f))
			- (volume_label_size.x / 2.0f * _volume_label->get_scale().x),
		slider_position.y - _volume_label->get_size().y * _volume_label->get_scale().y));
	//const float rotate_magnitude = 20.0f;
	//_volume_label->set_rotation_degrees(rotate_magnitude * slide_ratio - rotate_magnitude / 2.0f);

	auto master_index = _audio_server->get_bus_index("Master");
	_audio_server->set_bus_volume_db(master_index, Math::linear2db(slide_ratio));
	_current_volume_ratio = slide_ratio;
	emit_signal("volume_changed", slide_ratio);
}

void Client::_on_graphics_options_item_selected(int index)
{
	_current_graphics_level = index;
	emit_signal("graphics_quality_changed", index);
}

void Client::_on_back_button_pressed()
{
	set_paused(false);
}

void Client::_on_quit_button_pressed()
{
	UtilityFunctions::print("Quit received, exiting...");
	get_tree()->get_root()->call_deferred(
		"propagate_notification", NOTIFICATION_WM_CLOSE_REQUEST);
	get_tree()->quit(0);
}

void Client::_on_setup_language_options_item_selected(int index)
{
	// Cursed but will ensure that both selects are synchronised
	_language_options->select(index);
	_on_language_options_item_selected(index);
}

void Client::_on_language_options_item_selected(int index)
{
	switch (index) {
		case 1: // English
			_translation_server->set_locale("en");
			break;
		case 2: // English mad
			_translation_server->set_locale("en.mad");
			break;
		case 4: // Russian
			_translation_server->set_locale("ru");
			break;
		case 5: // Russian mad
			_translation_server->set_locale("ru.mad");
			break;
		case 7: // Turkish
			_translation_server->set_locale("tr");
			break;
		case 8: // Turkish mad
			_translation_server->set_locale("tr.mad");
			break;
	}
}

void Client::_on_setup_preset_button_pressed()
{
	_select_preset_label->set_visible(false);
}

void Client::_on_setup_confirm_button_pressed()
{
	if (_mobile_presets_button->is_pressed()) {
		_presets_platform = PresetsPlatform::MOBILE;
		// Auto set low graphics
		_graphics_options->select(0);
	}
	else if (_pc_presets_button->is_pressed()) {
		_presets_platform = PresetsPlatform::PC;
		// Auto set high graphics
		_graphics_options->select(1);
	}
	else {
		_select_preset_label->set_visible(true);
		return;
	}

	_setup_panel->set_visible(false);
}

int Client::get_player_id()
{
	return _player_id;
}

PlayerBody* Client::get_player_body()
{
	return _player_body;
}

String Client::get_current_phase_scene()
{
	return _current_phase_scene;
}

String Client::get_current_phase_event()
{
	return _current_phase_event;
}

PresetsPlatform Client::get_presets_platform()
{
	return _presets_platform;
}

HashMap<int, EntityPlayerBase*> Client::get_players()
{
	return _players;
}

EntityPlayerBase* Client::get_player(int id)
{
	if (_players.has(id)) {
		return _players[id];
	}

	return nullptr;
}

int Client::get_entity_id(Node* entity_node)
{
	// TODO: This is inefficient & scuffed - maintain a reverse HashMap instead
	for (auto &[id, entity] : _entities) {
		if (entity_node == entity) {
			return id;
		}
	}

	return -1;
}
