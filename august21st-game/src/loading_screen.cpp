#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/variant/signal.hpp>
#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/classes/web_socket_peer.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/classes/window.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/classes/mesh_instance3d.hpp>
#include <godot_cpp/variant/callable.hpp>
#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/classes/material.hpp>
#include <godot_cpp/classes/animation_player.hpp>
#include <godot_cpp/classes/audio_stream_player.hpp>
#include <godot_cpp/classes/audio_stream.hpp>
#include <godot_cpp/classes/item_list.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/classes/timer.hpp>
#include <godot_cpp/templates/list.hpp>
#include <dataproto_cpp/dataproto.hpp>
#include <godot_cpp/core/math.hpp>

#include "client.hpp"
#include "loading_screen.hpp"
#include "network_shared.hpp"
#include "node_shared.hpp"

using namespace std;
using namespace godot;
using namespace dataproto;
using namespace NetworkShared;

LoadingScreen::LoadingScreen()
{
}

LoadingScreen::~LoadingScreen()
{
}

void LoadingScreen::_bind_methods()
{
	ClassDB::bind_method(D_METHOD("_on_graphics_quality_changed", "level"),
		&LoadingScreen::_on_graphics_quality_changed);
	ClassDB::bind_method(D_METHOD("_on_flying_objects_player_animation_finished", "anim_name"),
		&LoadingScreen::_on_flying_objects_player_animation_finished);
	ClassDB::bind_method(D_METHOD("_on_song_player_finished"),
		&LoadingScreen::_on_song_player_finished);
	ClassDB::bind_method(D_METHOD("_on_retry_timer_timeout", "server_idx"),
		&LoadingScreen::_on_retry_timer_timeout);
	ClassDB::bind_method(D_METHOD("_on_packet_received", "packed_packet"),
		&LoadingScreen::_on_packet_received);
	ClassDB::bind_method(D_METHOD("_on_server_list_item_activated", "index"),
		&LoadingScreen::_on_server_list_item_activated);
}

void LoadingScreen::_ready()
{
	_engine = Engine::get_singleton();
	_resource_loader = ResourceLoader::get_singleton();

	_client = NodeShared::get_global_client(this);
	if (_client == nullptr) {
		UtilityFunctions::printerr("Could not get client: autoload singleton was null");
		set_process(false);
		return;
	}
	_client->connect("graphics_quality_changed", Callable(this, "_on_graphics_quality_changed"));

	_players_label = get_node<Label>("%PlayerCountLabel");

	_server_panel = get_node<Panel>("%ServerPanel");
	_server_panel->set_visible(true);
	_server_list = get_node<ItemList>("%ServerList");
	_server_list->connect("item_activated", Callable(this, "_on_server_list_item_activated"));
	_servers = List<LoadingServer*>();
	add_server("ws://localhost:8021");

	_song_player = get_node<AudioStreamPlayer>("%SongPlayer");
	_song_player->connect("finished", Callable(this, "_on_song_player_finished"));
	_current_song_label = get_node<Label>("%CurrentSongLabel");
	_current_loading_song = 0;
	play_song(_loading_songs[_current_loading_song]);

	_flying_objects_player = get_node<AnimationPlayer>("%FlyingObjectsPlayer");
	_flying_objects_player->connect("animation_finished", Callable(this, "_on_flying_objects_player_animation_finished"));
	_flying_objects_player->play("flying_objects");

	_tube = get_node<Node3D>("%Tube");
}

double LoadingScreen::get_next_retry_delay(LoadingServer* server)
{
	// Exponential backoff
	int retry_delay = INITIAL_SOCKET_RETRY_DELAY_SECONDS
		* Math::pow(INCREASE_SOCKET_RETRY_DELAY_FACTOR, server->attempt_count);
	if (retry_delay > MAX_SOCKET_RETRY_DELAY_SECONDS) {
		// Client is probably offline or issue is too severe to continue
		return Math_INF;
	}

	return retry_delay;
}

void LoadingScreen::_on_retry_timer_timeout(int server_idx)
{
	auto server = _servers[server_idx];
	try_connect_server_with_retry(server);
}

void LoadingScreen::try_connect_server_with_retry(LoadingServer* server)
{
	if (server == nullptr) {
		UtilityFunctions::print("Could not try reconnect to server: Server was null.");
		return;
	}

	auto socket = server->socket;
	// Reset as we are starting a new connection
	server->received_packets = false;
	auto socket_error = socket->connect_to_url(server->url);

	if (socket_error) {
		server->closed = true;
		server->attempt_count++;

		auto retry_delay = get_next_retry_delay(server);
		UtilityFunctions::print("Websocket connection to ", server->url,
			" failed: Retrying in ", retry_delay, " seconds.");
		auto retry_timer = server->retry_timer;
		retry_timer->set_wait_time(retry_delay);
		retry_timer->start();
	}
	else {
		server->closed = false;
	}
}

void LoadingScreen::add_server(String url)
{
	auto socket = Ref<WebSocketPeer>();
	socket.instantiate();

	auto retry_timer = memnew(Timer());
	add_child(retry_timer);

	auto server = memnew(LoadingServer({
		.url = url,
		.socket = socket,
		.retry_timer = retry_timer,
	}));

	// Add to server GUI
	auto icon_resource = _resource_loader->load("res://assets/rplace_gold.png");
	if (icon_resource->is_class("Texture2D")) {
		Ref<Texture2D> server_icon = icon_resource;
		auto server_description = String("Connecting to server....");
		auto id = _server_list->add_item(server_description, server_icon, false);
		_server_list->set_item_disabled(id, true);
		server->item_id = id;
	}

	// Setup reconnections & try connect to server
	_servers.push_back(server);
	auto server_index = _servers.size() - 1;
	retry_timer->connect("timeout", Callable(this, "_on_retry_timer_timeout")
		.bind(server_index));
	try_connect_server_with_retry(server);
}

void LoadingScreen::_on_server_list_item_activated(int index)
{
	for (auto server : _servers) {
		if (server->item_id != index) {
			continue;
		}
		server->current = true;
		_client->start_with_socket(server->socket);
		_server_panel->set_visible(false);
		break;
	}
}

void LoadingScreen::_process(double delta)
{
	for (auto i = 0; i < _servers.size(); i++) {
		auto server = _servers[i];
		// If we poll from the current server the client is using, we will
		// steal its packets
		if (server->current == true) {
			continue;
		}
		auto socket = server->socket;
		if (server->closed || !socket.is_valid()) {
			continue;
		}
		// Similar implementation to Client::poll_next_packets
		socket->poll();
		auto state = socket->get_ready_state();
		switch (state) {
			case WebSocketPeer::STATE_CONNECTING: {
				break;
			}
			case WebSocketPeer::STATE_OPEN: {
				while (socket->get_available_packet_count()) {
					// Reset retry vars on successful connection and receive
					server->attempt_count = 1;
					server->received_packets = true;
					_server_list->set_item_disabled(server->item_id, false);

					// Handle server list packets
					auto packed_packet = socket->get_packet();
					auto packet = BufReader((char*) packed_packet.ptr(), packed_packet.size());
					auto code = static_cast<ServerPacket>(packet.u8());
					switch (code) {
						case ServerPacket::SERVER_INFO: {
							auto duration = packet.u32();
							server->duration_s = duration;
							server->player_count = packet.u32();
							server->player_limit = packet.u32();
							auto phase_str = (string) packet.str();
							server->phase = String(phase_str.c_str());

							// Update GUI
							String server_description;
							auto int_hours = (int) duration / 3600;
							auto hours = String("{0}").format(Array::make(int_hours)).lpad(2, "0");
							auto minutes = String("{0}").format(Array::make((duration % 3600) / 60)).lpad(2, "0");
							auto seconds = String("{0}").format(Array::make(duration % 60)).lpad(2, "0");

							if (int_hours > 0) {
								server_description = String("{0}:{1}:{2} Players: {3}, Phase: {4}")
									.format(Array::make(hours, minutes, seconds, server->player_count, server->phase));
							}
							else {
								server_description = String("{0}:{1} Players: {2}, Phase: {3}")
									.format(Array::make(minutes, seconds, server->player_count, server->phase));
							}
							_server_list->set_item_text(server->item_id, server_description);
							break;
						}
						default: {
							break;
						}
					}
				}
				break;
			}
			case WebSocketPeer::STATE_CLOSING: {
				break;
			}
			case WebSocketPeer::STATE_CLOSED: {
				auto code = socket->get_close_code();
				auto reason = socket->get_close_reason();
				auto retry_delay = get_next_retry_delay(server);
				UtilityFunctions::print("Disconnected from serverlist socket ", server->url, ": Code: ",
					code, ", Reason: '", reason, "', Retrying in ", retry_delay, " seconds.");
				_server_list->set_item_disabled(server->item_id, true);
				server->closed = true;

				// Likely a failed connection, rather than an intentional server disconnnect, retry
				if (!server->received_packets || code == -1) {
					server->attempt_count++;

					// Attempt to reconnect
					auto retry_timer = server->retry_timer;
					retry_timer->set_wait_time(retry_delay);
					retry_timer->start();
				}
				break;
			}
		}
	}
}

void LoadingScreen::_on_packet_received(PackedByteArray packed_packet)
{
	/*auto packet = BufReader((char*) packed_packet.ptr(), packed_packet.size());
	uint8_t code = packet.u8();
	switch (code) {
		case ServerPacket::GAME_INFO: {
			auto players_waiting = packet.u32();
			auto formatted_count = String("Players waiting: {0}")
				.format(Array::make(players_waiting));
			_players_label->set_text(formatted_count);
			break;
        }
    }*/
}

void LoadingScreen::_on_graphics_quality_changed(int level)
{
	auto tube_mesh_node = _tube->get_child(0);
	if (tube_mesh_node != nullptr && tube_mesh_node->is_class("MeshInstance3D")) {
		auto tube_mesh = Object::cast_to<MeshInstance3D>(tube_mesh_node);
		auto tube_material_resource = _resource_loader->load(level == 0
			? "res://assets/tube_material_low.tres"
			: "res://assets/tube_material_high.tres");
		if (tube_material_resource.is_valid()) {
			const Ref<Material> tube_material = tube_material_resource;
			if (!tube_material.is_valid()) {
				UtilityFunctions::printerr("Failed to load tube material: resource was invalid");
			}
			else {
				tube_mesh->set_material_override(tube_material);
			}
		}
		else {
			UtilityFunctions::printerr("Failed to load tube material: file not found");
		}
	}
}

void LoadingScreen::_on_flying_objects_player_animation_finished(String anim_name)
{
	_flying_objects_player->play("flying_objects");
}

void LoadingScreen::_on_song_player_finished()
{
	_current_loading_song = (_current_loading_song + 1) % size(_loading_songs);
	play_song(_loading_songs[_current_loading_song]);
}

void LoadingScreen::play_song(LoadingSong song)
{
	_current_song_label->set_text(String::utf8("♪ {0} (by {1})").format(Array::make(song.name, song.author)));
	auto song_resource = _resource_loader->load(song.path);;
	_song_player->set_stream(song_resource);
	_song_player->play();
}
