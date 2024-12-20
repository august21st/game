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
#include <godot_cpp/classes/v_box_container.hpp>
#include <godot_cpp/classes/ray_cast3d.hpp>
#include <godot_cpp/classes/skeleton3d.hpp>
#include <godot_cpp/classes/nine_patch_rect.hpp>
#include <godot_cpp/classes/shader_material.hpp>
#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/classes/h_box_container.hpp>

#include "entity_player_base.hpp"
// Forward declare Client as it is only used as a pointer here
class Client;

using namespace godot;

class PlayerBody : public EntityPlayerBase {
	GDCLASS(PlayerBody, CharacterBody3D);

private:
	ResourceLoader* _resource_loader;
	// Won't be null - Class depends strongly on client
	Client* _client;
	ProjectSettings* _project_settings;
	Input* _player_input;
	float _gravity;
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
	Button* _action_button;
	Button* _chat_button;
	Button* _chat_close_button;
	Panel* _chat_panel;
	LineEdit* _chat_input;
	Button* _chat_send_button;
	VBoxContainer* _chat_messages_container;
	Label* _health_label;
	bool _climbing;
	Vector3 _spawn_position;
	int _update_tick;
	Node3D* _player_model;
	Skeleton3D* _skeleton;
	AnimationPlayer* _animation_player;
	HBoxContainer* _inventory_box;
	NinePatchRect*  _inventory_selector;
	Label* _inventory_selector_label;
	Ref<ShaderMaterial> _item_outline_material;

	int _stats_level;
	Label* _stats_label;
	void update_stats();

	// Item management
	RayCast3D*  _grab_ray;
	EntityItemBase* _hovered_item_entity;
	int scroll_inventory_current(int by);
	void set_inventory_current(int value) override;
	void set_mesh_next_pass_recursive(Node* root, Ref<Material> material);

	// Server
	Vector3 _last_packet_posiiton;
	Vector3 _last_packet_rotation;

	// Signal handlers
	void _on_revive_pressed();
	void _on_thumbstick_button_down();
	void _on_thumbstick_button_up();
	void _on_jump_button_down();
	void _on_jump_button_up();
	void _on_chat_button_pressed();
	void _on_chat_close_button_pressed();
	void _on_chat_close_tween_completed();
	void _on_packet_received(PackedByteArray packed_packet);
	void _on_chat_submit();
	void _on_stats_level_changed(int level);

	// Constants
	const float MAX_SPEED = 6.0f;
	const float MAX_CLIMB_SPEED = 2.0f;
	const float ACCELERATION = 12.0f;
	const float DECELERATION = 10.0f;
	const float JUMP_SPEED = 5.0f;
	const float MOUSE_SENSITIVITY = 0.001f;
	const float FALL_DAMAGE_THRESHOLD = -5.0f;
	const float FALL_DAMAGE_MIN = 4.0f;
	const int DEFAULT_HEALTH = 100;
	const float STILL_THRESHOLD  = 0.1;

protected:
	static void _bind_methods();

public:
	PlayerBody();
	~PlayerBody();
	void _ready() override;
	void _input(const Ref<InputEvent> &event) override;
	void _unhandled_input(const Ref<InputEvent> &event) override;
	void _physics_process(double delta) override;
	void _process(double delta) override;
	void die(String reason, String message="[center]Press revive to respawn...[/center]") override;
	void respawn(String phase_scene, Vector3 position) override;
	void take_damage(int damage);
	void set_climbing(bool climbing);
	void open_chat();
	void close_chat();
	void send_chat(String message);
	void set_spawn_position(Vector3 position);

	// Server calls (should not make server callbacks)
	void set_health(int value) override;
};
