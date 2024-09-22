#pragma once
#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/classes/label3d.hpp>
#include <godot_cpp/classes/mesh_instance3d.hpp>

#include "entity_player_base.hpp"

using namespace godot;

class EntityPlayer : public EntityPlayerBase {
	GDCLASS(EntityPlayer, EntityPlayerBase);

private:
	Label3D* _chat_name_label;
	MeshInstance3D* _healthbar_mesh;

protected:
	static void _bind_methods();

public:
	EntityPlayer();
	~EntityPlayer();
	void _ready() override;
	void set_chat_name(String name) override;
	String get_chat_name() override;
	void set_health(int value) override;
	int get_health() override;
	void set_model_variant(String value) override;
	String get_model_variant() override;
};
