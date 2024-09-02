#pragma once
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/classes/web_socket_peer.hpp>
#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/classes/animation_player.hpp>
#include <godot_cpp/classes/audio_stream_player.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/classes/item_list.hpp>
#include <godot_cpp/classes/timer.hpp>
#include <godot_cpp/templates/list.hpp>

#include "client.hpp"

using namespace godot;

struct LoadingSong {
	String name;
	String author;
	String path;
};

struct LoadingServer {
	// Connection
	String url = "";
	Ref<WebSocketPeer> socket = nullptr;
	int retry_count = 0;
	Timer* retry_timer = nullptr;
	bool closed = true;
	// Server list
	int duration_s = 0;
	int player_count = 0;
	String phase = "";
	int item_id = -1;
};

class LoadingScreen : public Node3D {
	GDCLASS(LoadingScreen, Node3D);

private:
	Engine* _engine;
	ResourceLoader* _resource_loader;
	Client* _client;
	Label* _players_label;
	ItemList* _server_list;
	Node3D* _tube;
	AnimationPlayer* _flying_objects_player;
	AudioStreamPlayer* _song_player;
	Label* _current_song_label;
	int _current_loading_song = 0;
	const LoadingSong _loading_songs[5] = {
		{ .name = "August 21st", .author = "SussZ", .path = "res://assets/august_21st.mp3" },
		{ .name = "{{}.mp3", .author = "Zubigri", .path = "res://assets/unclosed_braces.mp3" },
		{ .name = "Wastewater", .author = "NoNameGuy & SussZ", .path = "res://assets/wastewater.mp3" },
		{ .name = "Zubigri #3 - Villager", .author = "SussZ", .path = "res://assets/zubigri_3_villager.mp3" },
		{ .name = "Zubigri #6 - Speed of life", .author = "SussZ", .path = "res://assets/zubigri_6_speed_of_life.mp3" }
	};
	List<LoadingServer*> _servers;
    const int MAX_SOCKET_RETRY_DELAY_SECONDS = 60;
    const float INITIAL_SOCKET_RETRY_DELAY_SECONDS = 0.5f;
	void add_server(String url);
	void try_reconnect_server(LoadingServer* server);
	void _on_retry_timer_finished(int server_idx);
	void _on_packet_received(PackedByteArray packed_packet);
	void _on_graphics_quality_changed(int level);
	void _on_flying_objects_player_animation_finished(String anim_name);
	void _on_song_player_finished();
	void play_song(LoadingSong song);

protected:
	static void _bind_methods();

public:
	LoadingScreen();
	~LoadingScreen();
	void _ready() override;
	void _process(double delta) override;
};
