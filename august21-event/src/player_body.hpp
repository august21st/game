#pragma once
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/input.hpp>
#include <godot_cpp/classes/input_event.hpp>
#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/variant/vector3.hpp>
#include <godot_cpp/classes/character_body3d.hpp>
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/classes/camera3d.hpp>
#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/classes/line_edit.hpp>
#include <godot_cpp/classes/button.hpp>
#include <godot_cpp/classes/panel.hpp>
#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/classes/texture_rect.hpp>
#include <godot_cpp/classes/rich_text_label.hpp>
#include <godot_cpp/classes/animation_player.hpp>
#include <godot_cpp/classes/performance.hpp>

using namespace godot;

class PlayerBody : public CharacterBody3D {
	GDCLASS(PlayerBody, CharacterBody3D);

private:
	Engine* _engine;
	Performance* _performance;
	ProjectSettings* _project_settings;
	Input* _player_input;
	float _gravity;
	int _health;
	bool _is_dead;
	Vector3 _velocity;
	Camera3D* _camera;
	AnimationPlayer* _camera_animation_player;
	Node3D* _camera_pivot;
	Control* _death_panel;
	Label* _death_title_label;
	RichTextLabel* _death_message_label;
	Button* _grab_button;
	Button* _revive_button;
	TextureRect* _thumbstick_panel;
	Button* _thumbstick_button;
	bool _dragging_thumbstick;
	Vector2 _thumbstick_direction;
	Button* _jump_button;
	bool _jump_pressed;
	Button* _chat_button;
	Button* _chat_close_button;
	Panel* _chat_panel;
	LineEdit* _chat_input;
	Button* _chat_send_button;
	Label * _stats_label;
	Label* _health_label;
	bool _stats_enabled;
	bool _climbing;

protected:
	static void _bind_methods();
	void set_stats_enabled(bool enable);
	void update_stats();
	void update_hotbar();

public:
	PlayerBody();
	~PlayerBody();
	Vector3 spawn_position;
	void _ready() override;
	void _input(const Ref<InputEvent> &event) override;
	void _unhandled_input(const Ref<InputEvent> &event) override;
	void _physics_process(double delta) override;
	void _process(double delta) override;
	void _on_revive_pressed();
	void _on_thumbstick_button_down();
	void _on_thumbstick_button_up();
	void _on_jump_button_down();
	void _on_jump_button_up();
	void _on_chat_button_pressed();
	void _on_chat_close_button_pressed();
	void _on_chat_close_tween_completed();
	void die(String death_title = "YOU DIED", String death_message = "[center]Press revive to respawn...[/center]");
	void respawn(Vector3 position);
	void take_damage(int damage);
	void set_climbing(bool climbing);
	void open_chat();
	void close_chat();
};
