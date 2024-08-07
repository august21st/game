#pragma once
#include <godot_cpp/classes/panel.hpp>
#include <godot_cpp/classes/button.hpp>
#include <godot_cpp/classes/performance.hpp>
#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/classes/os.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/display_server.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/h_slider.hpp>
#include <godot_cpp/classes/web_socket_peer.hpp>
#include <godot_cpp/variant/packed_byte_array.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/classes/input_event.hpp>
#include <dataproto_cpp/dataproto.hpp>
#include <godot_cpp/classes/audio_server.hpp>
#include <godot_cpp/classes/option_button.hpp>
#include <godot_cpp/classes/input.hpp>
#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/classes/rich_text_label.hpp>
#include <godot_cpp/templates/hash_map.hpp>

// WORKAROUND: Forward declare to fix circular dependency
class PlayerBody;
#include "player_body.hpp"
#include "entity_player.hpp"

using namespace godot;
using namespace dataproto;

// Global autoload singleton for client, manages global
// client state, like handling websocket connection and settings
class Client : public Node {
	GDCLASS(Client, Node);

private:
	OS *_os;
	Engine *_engine;
	DisplayServer* _display_server;
	AudioServer* _audio_server;
	Performance* _performance;
	ResourceLoader* _resource_loader;
	Input* _player_input;
	bool _is_server;
	Node3D* _client_scene;
	Control* _client_gui;
	Label* _stats_label;
	bool _stats_enabled;
	void set_stats_enabled(bool enable);
	void update_stats();
	Panel* _pause_panel;
	Button* _pause_button;
	void _on_pause_button_pressed();
	bool _paused;
	void set_paused(bool pause);
	RichTextLabel* _volume_label;
	void _on_volume_slider_drag_started();
	void _on_volume_slider_drag_ended(bool value_changed);
	void _on_volume_slider_value_changed(float value);
	HSlider* _volume_slider;
	OptionButton* _graphics_options;
	int _current_graphics_level;
	void _on_graphics_options_item_selected(int index);
	Button* _back_button;
	Button* _close_button;
	void _on_back_button_pressed();
	Button* _quit_button;
	void _on_quit_button_pressed();
	Ref<WebSocketPeer> _socket;
	vector<PackedByteArray> poll_next_packets();
	bool _socket_closed;
	String _current_phase_scene;
	String _current_phase_event;
	PlayerBody* _player_body;
	HashMap<int, Node*> _entities;
	HashMap<int, EntityPlayer*> _players;
	double round_decimal(double value, int places);
	const String _volume_comments[101] = {
		"Goes hard on mute 🗣️ 🗣️", // 0
		"Interesting choice...", // 1
		"Trucks having se-", // 2
		"Why only tree who even...", // 3
		"Stands 4", // 4
		"Aren't you just checking all?", // 5
		"Colors gey", // 6
		"<- IS THAT A $^£&*1NG NUMBER!!!???", // 7
		"EIGHTEIGHTEIGHTEIGHTEIGHTEIGHTEIGHTEIGHT", // 8
		"Forget", // 9
		"Percents of the way as well", // 10
		"About it", // 11
		"Ur mum", // 12
		"Uuuugh", // 13
		"Maybe i have something to say idk", // 14
		"I am Zubigri and i writing this at 6:00 UTC+5 4 AUGUST 2024", // 15
		"Also uuum fid you know that hollow cube...", // 16
		"...topological have 5 holes?", // 17
		"+", // 18
		"is not equal 2 9 + 10", // 19
		"Pum pum pum that's pretty quiet", // 20
		"AUGUST (CHAOS! CHAOS! CHAOS!)", // 21
		"Some of texts was written by Zekiah", // 22
		"Not too much tho", // 23
		"i like random texts", // 24
		"1/4", // 25
		"JK JK here: 8, 44, 80, 88, 98, kinda 9, 11, 20, 21", // 26
		"🪑🔥", // 27
		"Can we just skip it?", // 28
		"Please go one up, so unsatisfying", // 29
		"At least it's a round number", // 30
		"Fact: 100-31=69, i also like gallium", // 31
		"Original rplace colour count btw", // 32
		"Hello kitty city", // 33
		"is an infamous number", // 34
		"Still quiet lol", // 35
		"6*6", // 36
		"I think WW2 started this year", // 37
		"Factual and peer reviewed", // 38
		"🤫🧏", // 39
		"How many do we have? Fourty, 0_0, thanks you", // 40
		"I mean what chance of randomly finding this?", // 41
		"FOURTY TWO", // 42
		"Mmm... Wat", // 43
		"Normal", // 44
		"Fire in the hole", // 45
		"💶💶😂😂", // 46
		"Go one up for spam", // 47
		"Sksheiamhksnsje (told ya so)", // 48
		"Just go to 50. Please just go to 50", // 49
		"Is the glass half empty or half full?", // 50
		"ВОРОВСТВО", // 51
		"👁️‍🗨️ Подними", // 52
		"Я не ебал слонов", // 53
		"54", // 54
		"fifyfife", // 55
		"🐳", // 56
		"Εκ του ασφαλούς και την ανάπτυξη του παιδιού σας", // 57
		"Please share 21 AUGUST (ANYTHING)", // 58
		"What did you expect to be here?", // 59
		"You thought that something will be here?", // 60
		"Still kinda low volume", // 61
		"If you chose this one u love boys", // 62
		"🐟🧊", // 63
		"Petition to destroy all 32 bit devices", // 64
		"Why 6 afraid of 7?", // 65
		"666", // 66
		"666?", // 67
		"Empty messages", // 68
		"Go 38 lower for funny fact lol", // 69
		"Seventeen", // 70
		"Mhm", // 71
		"Me here", // 72
		"¢", // 73
		"$", // 74
		"3/4", // 75
		"Ugh A", // 76
		"Ugh B", // 77
		"Make more content for 21 AUGUST, @everyone", // 78
		"Ugh C", // 79
		"Can we get much higher?", // 80
		"Keep going!!!", // 81
		"The voice in your head says Keep going!!!!", // 82
		"👁️", // 83
		"The first number 2x the second", // 84
		"Not zero", // 85
		"...", // 86
		"NaN better anyway", // 87
		"Oh boy...", // 88
		"Oh girl...", // 89
		"10% left to go", // 90
		"9% left to go", // 91
		"21 August soon as well, for me, writing this", // 92
		"about this long spent coding these msgs", // 93
		"Almost here", // 94
		"Few left", // 95
		"Two left", // 96
		"Basically max volume", // 96
		"We losing our hearing with this one 🔥 🔥 🔥", // 98
		"You are monster if you will stop now", // 99
		"End." // 100
	};

protected:
	static void _bind_methods();

public:
	Client();
	~Client();
	void _ready() override;
	void _input(const Ref<InputEvent> &event) override;
	void _process(double delta) override;
	Ref<WebSocketPeer> get_socket();
	Error send(BufWriter* packet);
	Error send(const char* data, size_t size);
	template<typename T> T* get_current_scene();
	template<typename T> T* instance_scene(String scene_path);
	void orphan_node(Node* node);
	void init_socket_client(String url);
	void set_volume(float volume);
	Error load_scene(String scene_path, Node** out_scene);
	Error change_scene(String scene_path);
	String get_current_phase_scene();
	String get_current_phase_event();
	PlayerBody* get_player_body();
};
