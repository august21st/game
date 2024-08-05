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

#include "client.hpp"
#include "player_body.hpp"
#include "network_shared.hpp"

using namespace godot;
using namespace dataproto;
using namespace NetworkShared;

Client::Client() : _socket(Ref<WebSocketPeer>()), _socket_closed(true)
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

	ADD_SIGNAL(MethodInfo("packet_received",
		PropertyInfo(Variant::PACKED_BYTE_ARRAY, "packed_packet")));
	ADD_SIGNAL(MethodInfo("graphics_quality_changed",
		PropertyInfo(Variant::INT, "level")));

}

void Client::_ready()
{
	_os = OS::get_singleton();
	_engine = Engine::get_singleton();
	_performance = Performance::get_singleton();
	_display_server = DisplayServer::get_singleton();
	bool is_server = _os->has_feature("dedicated_server") || _display_server->get_name() == "headless";
	if ((_engine &&_engine->is_editor_hint()) || is_server) {
		set_process(false);
		return;
	}
	UtilityFunctions::print("Starting as client...");

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

	_back_button = get_node<Button>("%BackButton");
	_back_button->connect("pressed", Callable(this, "_on_back_button_pressed"));

	_quit_button = get_node<Button>("%QuitButton");
	_quit_button->connect("pressed", Callable(this, "_on_quit_button_pressed"));

	_player_body = instance_player_body();

	init_socket_client("ws://localhost:8082");
	change_scene("res://scenes/loading_screen.tscn");
}

Node* Client::get_current_scene()
{
	return get_tree()->get_current_scene();
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

	PackedByteArray packed_data;
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
	if (_stats_enabled) {
		update_stats();
	}

	if (!_socket_closed && _socket.is_valid()) {
		auto packed_packets = poll_next_packets();
		for (PackedByteArray packed_packet : packed_packets) {
			auto packet = BufReader((char*) packed_packet.ptr(), packed_packet.size());
			uint8_t code = packet.u8();
			switch (code) {
				case ServerPacket::SET_PHASE: {
					auto phase_name_str = (string) packet.str();
					auto phase_name = String(phase_name_str.c_str());
					auto phase_scene = phase_name.split(":")[0];
					if (phase_scene == "inro") {
						change_scene("res://scenes/intro.tscn");
					}
					else if (phase_scene == "roof") {
						change_scene("res://scenes/roof.tscn");
					}
					else if (phase_scene == "end") {
						change_scene("res://scenes/end.tscn");
					}
					else {
						UtilityFunctions::printerr("Could not set phase to ", phase_name, ": unknown phase");
					}
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

Error Client::load_scene(String scene_path, Node** out_scene_instance)
{
	auto control_scene_resource = _resource_loader->load(scene_path);
	if (!control_scene_resource.is_valid() || !control_scene_resource->is_class("PackedScene")) {
		UtilityFunctions::printerr("Failed to load scene control scene: file not found");
		return Error::ERR_FILE_NOT_FOUND;
	}

	Ref<PackedScene> control_scene = control_scene_resource;
	if (!control_scene.is_valid() || !control_scene->can_instantiate()) {
		UtilityFunctions::printerr("Failed to load control scene: resource was invalid");
		return Error::ERR_INVALID_DATA;
	}

	auto scene_instance = control_scene->instantiate();
	*out_scene_instance = scene_instance;
	return Error::OK;
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

	_client_scene->add_child(scene_instance);
	return Error::OK;
}

PlayerBody* Client::instance_player_body()
{
	Node* scene_instance;
	auto load_error = load_scene("res://scenes/player_body.tscn", &scene_instance);
	if (load_error != Error::OK || !scene_instance->is_class("PlayerBody")) {
		return nullptr;
	}

	auto player_body = (PlayerBody*) scene_instance;
	return player_body;
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
	set_volume(slide_ratio);
}

void Client::set_volume(float volume_ratio)
{
	auto master_index = _audio_server->get_bus_index("Master");
	_audio_server->set_bus_volume_db(master_index, volume_ratio * -60.0f);
	if (_volume_slider != nullptr) {
		_volume_slider->set_value(volume_ratio * 100.0f);
	}
}

void Client::_on_graphics_options_item_selected(int index)
{
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

PlayerBody* Client::get_player_body()
{
	return _player_body;
}
