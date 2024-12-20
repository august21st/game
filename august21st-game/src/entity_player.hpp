#pragma once
#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/classes/label3d.hpp>
#include <godot_cpp/classes/mesh_instance3d.hpp>
#include <godot_cpp/templates/list.hpp>
#include <godot_cpp/variant/packed_byte_array.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/vector3.hpp>

#include "entity_player_base.hpp"
class GameRoot;

using namespace godot;

class EntityPlayer : public EntityPlayerBase {
	GDCLASS(EntityPlayer, EntityPlayerBase);

private:
	Label3D* _chat_name_label;
	MeshInstance3D* _healthbar_mesh;
	String _current_animation;
	String _current_phase_scene;
	Vector3 _velocity;
	GameRoot* _game_root;
	const String _model_variants[8] = {
		"lightblue",
		"navy",
		"green",
		"purple",
		"grey",
		"brown",
		"orangered",
		"gold"
	};


protected:
	static void _bind_methods();

public:
	EntityPlayer();
	~EntityPlayer();
	void _ready() override;
	void _physics_process(double delta) override;

	void _on_packet_received(int sender_id, PackedByteArray packed_packet);

	void update_movement(Vector3 position, Vector3 velocity, Vector3 rotation);
	void broadcast_movement();

	void action_grab(int entity_id);
	void action_drop();
	void action_use();
	void action_switch(int inventory_i);

	void take_damage(int value);
	void set_health(int value) override;
	int get_health() override;
	void broadcast_health();

	void die(String reason="", String message="") override;
	void broadcast_die();

	void set_chat_name(String name) override;
	String get_chat_name() override;
	void broadcast_chat_name();

	void set_model_variant(String value) override;
	String get_model_variant() override;
	void broadcast_model_variant();

	void respawn(String phase_scene, Vector3 position) override;
	void broadcast_respawn();
};
