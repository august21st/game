#pragma once
#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/label3d.hpp>
#include <godot_cpp/classes/mesh_instance3d.hpp>

using namespace godot;

class EntityPlayer : public Node3D {
	GDCLASS(EntityPlayer, Node3D);

private:
	Engine* _engine;
	Label3D* _chat_name_label;
	MeshInstance3D* _healthbar_mesh;
	int _player_id;
	String _chat_name;
	int _health;
	bool ready_and_connected();

protected:
	static void _bind_methods();

public:
	EntityPlayer();
	~EntityPlayer();
	void _ready() override;
	void set_chat_name(String name);
	String get_chat_name();
	void set_health(int value);
	int get_health();
};
