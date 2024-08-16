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

#include "client.hpp"
#include "godot_cpp/core/math.hpp"
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
	ClassDB::bind_method(D_METHOD("_on_language_options_item_selected", "index"),
		&Client::_on_language_options_item_selected);

	ADD_SIGNAL(MethodInfo("packet_received",
		PropertyInfo(Variant::PACKED_BYTE_ARRAY, "packed_packet")));
	ADD_SIGNAL(MethodInfo("graphics_quality_changed",
		PropertyInfo(Variant::INT, "level")));
	ADD_SIGNAL(MethodInfo("volume_changed",
		PropertyInfo(Variant::INT, "volume_ratio")));
	ADD_SIGNAL(MethodInfo("current_scene_ready"));
}

void Client::_ready()
{
	_os = OS::get_singleton();
	_engine = Engine::get_singleton();
	if (_engine &&_engine->is_editor_hint()) {
		return;
	}

	_performance = Performance::get_singleton();
	_display_server = DisplayServer::get_singleton();
	_translation_server = TranslationServer::get_singleton();
	_is_server = _os->has_feature("dedicated_server") || _display_server->get_name() == "headless";

	_resource_loader = ResourceLoader::get_singleton();
	_audio_server = AudioServer::get_singleton();
	_player_input = Input::get_singleton();

	_client_scene = get_node<Node3D>("%ClientScene");
	_client_gui = get_node<Control>("%ClientGui");
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

	_setup_language_options = get_node<OptionButton>("%SetupLanguageOptions");
	_setup_language_options->connect("item_selected", Callable(this, "_on_language_options_item_selected"));

	_setup_confirm_button = get_node<Button>("%SetupConfirmButton");
	_setup_confirm_button->connect("pressed", Callable(this, "_on_setup_confirm_button_pressed"));

	_entities = { };
	_players = { };

	if (_is_server) {
		UtilityFunctions::print("Starting loopback client");
	}
	else {
		UtilityFunctions::print("Starting as client...");
		PlayerBody* player_body_scene = nullptr;
		auto load_error = load_scene_strict<PlayerBody>("res://scenes/player_body.tscn", &player_body_scene);
		if (load_error != Error::OK || player_body_scene == nullptr) {
			UtilityFunctions::print("Couldn't start client, player body scene failed to load");
			return;
		}
		_player_body = player_body_scene;

		init_socket_client("ws://localhost:8082");
		change_scene("res://scenes/loading_screen.tscn");
	}
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

void Client::init_socket_client(String url)
{
	_socket = Ref<WebSocketPeer>();
	_socket.instantiate();
	_socket->connect_to_url(url);
	set_fail_function([](const void *reason)
	{
		auto str_reason = static_cast<const char*>(reason);
		UtilityFunctions::printerr(String("dataproto error: {0}").format(Array::make(str_reason)));
	});
	_socket_closed = false;
	set_process(true);
}

Ref<WebSocketPeer> Client::get_socket()
{
	return _socket;
}

Error Client::send(BufWriter* packet)
{
	send(packet->data(), packet->size());
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

vector<PackedByteArray> Client::poll_next_packets()
{
	auto packets = vector<PackedByteArray>();
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
			auto formatted_log = String("WebSocket closed with code: {0}, reason \"{1}\". Clean: {2}")
				.format(Array::make(code, reason, code != -1));
			UtilityFunctions::printerr(formatted_log);
			_socket_closed = true;

			// TODO: try to re-establish connection with cooldown
			//UtilityFunctions::print("Attemping to re-establish WebSocket connection...");
			//init_socket_client("ws://localhost:8082");
			break;
	}

	return packets;
}

void Client::_input(const Ref<InputEvent> &event)
{
	if (_engine->is_editor_hint()) {
		return;
	}

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
	if (_engine->is_editor_hint()) {
		return;
	}

	if (_stats_enabled) {
		update_stats();
	}

	if (!_socket_closed && _socket.is_valid()) {
		auto packed_packets = poll_next_packets();
		for (PackedByteArray packed_packet : packed_packets) {
			auto packet = BufReader((char*) packed_packet.ptr(), packed_packet.size());
			uint8_t code = packet.u8();
			switch (code) {
				case ServerPacket::GAME_INFO: {
					auto players_waiting = packet.u32();
					auto player_id = packet.u32();
					_player_id = player_id;
					break;
				}
				case ServerPacket::PLAYERS_INFO: {
					auto player_count = packet.u16();
					for (auto i = 0; i < player_count; i++) {
						auto id = packet.u32();
						auto user_int_id = packet.u32();
						auto chat_name_str = (string) packet.str();
						auto chat_name = String(chat_name_str.c_str());

						if (id == _player_id) {
							// TODO: Apply to self
						}
						else {
							EntityPlayer* player_entity = nullptr;
							auto load_error = load_scene_strict<EntityPlayer>("res://scenes/entity_player.tscn", &player_entity);
							if (load_error != Error::OK || player_entity == nullptr) {
								UtilityFunctions::print("Failed to insert entity player: entity player scene failed to load.");
								return;
							}
							player_entity->set_chat_name(chat_name);
							_players.insert(id, player_entity);
						}
					}
					break;
				}
				case ServerPacket::UPDATE_PLAYER_MOVEMENT: {
					auto player_id = packet.u32();
					if (!_players.has(player_id)) {
						if (player_id != _player_id) {
							UtilityFunctions::printerr("Could not update player ",
								player_id, ": player entity not found");
						}
						break;
					}
					auto phase_scene_str = (string) packet.str();
					auto phase_scene = String(phase_scene_str.c_str());
					if (phase_scene != _current_phase_scene) {
						// Player is not in our world, ignore it
						break;
					}

					auto player = _players[player_id];
					auto position = Vector3(packet.f32(), packet.f32(), packet.f32());
					auto rotation = Vector3(packet.f32(), packet.f32(), packet.f32());
					auto current_animation_str = (string) packet.str();
					auto current_animation = String(current_animation_str.c_str());

					if (player_id == _player_id) {
						// TODO: Apply changes from server to us
					}
					else {
						auto current_scene_node = _client_scene->get_child(0);
						auto player_parent = player->get_parent();
						if (player_parent != current_scene_node) {
							orphan_node(player);
							current_scene_node->add_child(player);
						}

						player->set_position(position);
						player->set_rotation(rotation);
					}
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
					auto player = _players[player_id];
					auto health = packet.u32();

					if (player_id == _player_id) {
						// TODO: Apply changes from server to us
					}
					else {
						player->set_health(health);
					}
					break;
				}
				case ServerPacket::ENTITIES_INFO: {
					auto entity_count = packet.u16();
					for (auto i = 0; i < entity_count; i++) {
						auto id = packet.u32();
						auto parent_scene_str = (string) packet.str();
						auto parent_scene = String(parent_scene_str.c_str());

						auto entity_node = read_entity_data(&packet);
						if (entity_node == nullptr) {
							UtilityFunctions::print("Failed to create entity ", id, ": failed to decode entity data");
							continue;
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
				case ServerPacket::UPDATE_ENTITY: {
					break; // TODO: reimplement
					auto id = packet.u32();
					if (!_entities.has(id)) {
						UtilityFunctions::print("Couldn't update entity with id ",
							id, ": entity not found.");
						break;
					}
					auto entity = _entities[id];

					auto property_count = packet.u8();
					for (auto i = 0; i < property_count; i++) {
						auto property_str = (string) packet.str();
						auto property = String(property_str.c_str());
						auto value_data = packet.str();
						auto temp_buffer = PackedByteArray();
						temp_buffer.resize(value_data.size);
						memcpy(temp_buffer.ptrw(), value_data.data, value_data.size);
						auto value = packed_packet.decode_var(0);
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

					if (phase_scene == "loading_screen") {
						change_scene("res://scenes/loading_screen.tscn");
					}
					if (phase_scene == "intro") {
						change_scene("res://scenes/intro.tscn");
					}
					else if (phase_scene == "roof") {
						if (_current_phase_scene != phase_scene) {
							change_scene("res://scenes/roof.tscn");
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
							change_scene("res://scenes/end.tscn");
						}

						auto end_scene = get_current_scene_strict<End>();
						if (_player_body->get_parent() != end_scene) {
							orphan_node(_player_body);
							end_scene->spawn_player(_player_body);
						}
						end_scene->run_phase_event(phase_event);
					}
					else {
						UtilityFunctions::printerr("Could not set phase to ",
							phase, ": unknown phase");
					}
					_current_phase_scene = phase_scene;
					_current_phase_event = phase_event;
					break;
				}
				default: {
					emit_signal("packet_received", packed_packet);
					break;
				}
			}
		}
	}
}

void Client::set_stats_enabled(bool enable)
{
	_stats_enabled = enable;
	if (_stats_label != nullptr) {
		_stats_label->set_visible(enable);
	}
}

void Client::update_stats()
{
	if (_stats_label == nullptr) {
		return;
	}

	auto fps = _engine->get_frames_per_second();
	auto physics_process = _performance->get_monitor(Performance::Monitor::TIME_PHYSICS_PROCESS);
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
	auto stats_string = String("FPS: {0}\nPhysics process: {1}\nscene objects: "
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

Error Client::change_scene(String scene_path)
{
	Node* scene_instance;
	auto load_error = load_scene(scene_path, &scene_instance);
	if (load_error != Error::OK) {
		return load_error;
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
}

void Client::_on_volume_slider_value_changed(float value)
{
	auto volume_comment = _volume_comments[(int) Math::floor(value)];
	_volume_label->set_text(String("{0}% ({1})").format(Array::make(value, volume_comment)));
	_volume_label->set_size(_volume_label->get_minimum_size());
	auto slide_ratio = value / 100.0f;
	auto volume_label_size = _volume_label->get_size();
	_volume_label->set_pivot_offset(Vector2(
		volume_label_size.x / 2, volume_label_size.y));
	_volume_label->set_scale(
		Vector2(0.4f, 0.4f) * slide_ratio + Vector2(0.8f, 0.8f));
	_volume_label->set_rotation_degrees(20 * slide_ratio - 10.0f);
	auto slider_position = _volume_slider->get_global_rect().get_position();
	auto slider_size = _volume_slider->get_size();
	_volume_label->set_global_position(Vector2(
		slider_position.x + (slide_ratio * slider_size.x) - (volume_label_size.x / 2.0f),
		slider_position.y - 70.0f));

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
	UtilityFunctions::print("Exiting...");
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
		// Auto set low graphics
		_graphics_options->select(0);
	}
	else if (_pc_presets_button->is_pressed()) {
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
