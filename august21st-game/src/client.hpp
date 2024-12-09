#pragma once
#include <godot_cpp/classes/panel.hpp>
#include <godot_cpp/classes/button.hpp>
#include <godot_cpp/classes/performance.hpp>
#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/classes/os.hpp>
#include <godot_cpp/classes/engine.hpp>
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
#include <godot_cpp/classes/texture_rect.hpp>
#include <godot_cpp/classes/h_box_container.hpp>
#include <godot_cpp/classes/translation_server.hpp>
#include <godot_cpp/classes/java_script_bridge.hpp>
#include <godot_cpp/templates/list.hpp>
#include <godot_cpp/classes/timer.hpp>

#include "entity_player_base.hpp"
// Forward declare to fix circular dependency
class PlayerBody;
#include "player_body.hpp"

using namespace godot;
using namespace dataproto;

enum PresetsPlatform {
	MOBILE,
	PC
};

enum PhaseSceneLoadStatus {
	UNLOADED,
	REQUESTED,
	LOADED
};

struct PhaseSceneInfo {
	String scene_path;
	PhaseSceneLoadStatus load_status;
	Node* scene;
};

// Global autoload singleton for client, manages global
// client state, like handling websocket connection and settings
class Client : public Node {
	GDCLASS(Client, Node);

private:
	OS* _os;
	Engine* _engine;
	TranslationServer* _translation_server;
	AudioServer* _audio_server;
	Performance* _performance;
	ResourceLoader* _resource_loader;
	Input* _player_input;
	JavaScriptBridge* _js_bridge;
	bool _is_server;
	Node3D* _client_scene;
	// Interface
	Control* _client_gui;
	Label* _stats_label;
	bool _stats_enabled;
	void set_stats_enabled(bool enable);
	void update_stats();
	// Pause panel
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
	int _current_volume_ratio;
	OptionButton* _graphics_options;
	int _current_graphics_level;
	void _on_graphics_options_item_selected(int index);
	OptionButton* _language_options;
	void _on_setup_language_options_item_selected(int index);
	void _on_language_options_item_selected(int index);
	Button* _back_button;
	Button* _close_button;
	void _on_back_button_pressed();
	Button* _quit_button;
	void _on_quit_button_pressed();
	// Setup panel
	Panel* _setup_panel;
	RichTextLabel* _select_preset_label;
	Button* _mobile_presets_button;
	Button* _pc_presets_button;
	void _on_setup_preset_button_pressed();
	PresetsPlatform _presets_platform;
	OptionButton* _setup_language_options;
	Button* _setup_confirm_button;
	void _on_setup_confirm_button_pressed();
	// Alert panel
	Panel* _alert_panel;
	Label* _alert_label;
	Button* _alert_close_button;
	void _on_alert_close_button_pressed();
	void show_alert(String message);
	// Networking
	Ref<WebSocketPeer> _socket;
	List<PackedByteArray> poll_next_packets();
	bool _socket_closed;
	String _current_phase_scene;
	void _on_current_scene_ready();
	String _current_phase_event;
	int _player_id;
	PlayerBody* _player_body;
	HashMap<int, Node*> _entities;
	HashMap<int, EntityPlayerBase*> _players;
	HashMap<String, PhaseSceneInfo*> _phase_scenes;
	Error register_phase_scene(String identifier, String path);
	void _on_player_entity_ready(int id, String chat_name, String model_variant);
	double round_decimal(double value, int places);
	const String _volume_comments[101] = {
		String::utf8("Goes hard on mute 🗣️ 🗣️"), // 0
		"Interesting choice...", // 1
		"Trucks having se-", // 2
		"Why only tree who even...", // 3
		"Stands 4", // 4
		"Aren't you just checking all?", // 5
		"Colors gey", // 6
		String::utf8("<- IS THAT A $^£&*1NG NUMBER!!!???"), // 7
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
		String::utf8("🪑🔥"), // 27
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
		String::utf8("🤫🧏"), // 39
		"How many do we have? Fourty, 0_0, thanks you", // 40
		"I mean what chance of randomly finding this?", // 41
		"FOURTY TWO", // 42
		"Mmm... Wat", // 43
		"Normal", // 44
		"Fire in the hole", // 45
		String::utf8("💶💶😂😂"), // 46
		"Go one up for spam", // 47
		"Sksheiamhksnsje (told ya so)", // 48
		"Just go to 50. Please just go to 50", // 49
		"Is the glass half empty or half full?", // 50
		String::utf8("ВОРОВСТВО"), // 51
		String::utf8("👁️‍🗨️ Подними"), // 52
		String::utf8("Я не ебал слонов"), // 53
		"54", // 54
		"fifyfife", // 55
		String::utf8("🐳"), // 56
		String::utf8("Εκ του ασφαλούς και την ανάπτυξη του παιδιού σας"), // 57
		"Please share 21 AUGUST (ANYTHING)", // 58
		"What did you expect to be here?", // 59
		"You thought that something will be here?", // 60
		"Still kinda low volume", // 61
		"If you chose this one u love boys", // 62
		String::utf8("🐟🧊"), // 63
		"A full STACK!", // 64
		"Why 6 afraid of 7?", // 65
		"666", // 66
		"666?", // 67
		"Empty messages", // 68
		"Go 38 lower for funny fact lol", // 69
		"Seventeen", // 70
		"Mhm", // 71
		"Me here", // 72
		String::utf8("¢"), // 73
		"$", // 74
		"3/4", // 75
		"Ugh A", // 76
		"Ugh B", // 77
		"Make more content for 21 AUGUST, @everyone", // 78
		"Ugh C", // 79
		"Can we get much higher?", // 80
		"Keep going!!!", // 81
		"The voice in your head says Keep going!!!!", // 82
		String::utf8("👁️"), // 83
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
		String::utf8("We losing our hearing with this one 🔥 🔥 🔥"), // 98
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
	void start_with_socket(Ref<WebSocketPeer> socket);
	Ref<WebSocketPeer> get_socket();
	Error send(const BufWriter& packet);
	Error send(const char* data, size_t size);
	Node* get_current_scene();
	template<typename T> T* get_current_scene_strict();
	Error change_scene(String identifier);
	String get_current_phase_scene();
	String get_current_phase_event();
	PresetsPlatform get_presets_platform();
	int get_player_id();
	PlayerBody* get_player_body();
	HashMap<int, EntityPlayerBase*> get_players();
	EntityPlayerBase* get_player(int id);
	int get_entity_id(Node* entity);
};
