#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/variant/signal.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/variant/callable.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/method_bind.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/classes/window.hpp>
#include <godot_cpp/classes/directional_light3d.hpp>
#include <godot_cpp/classes/label3d.hpp>
#include <godot_cpp/classes/mesh_instance3d.hpp>
#include <godot_cpp/classes/mesh.hpp>
#include <godot_cpp/classes/quad_mesh.hpp>
#include <dataproto_cpp/dataproto.hpp>

#include "entity_player.hpp"


using namespace godot;
using namespace dataproto;

EntityPlayer::EntityPlayer() : _chat_name_label(nullptr), _healthbar_mesh(nullptr)
{
}

EntityPlayer::~EntityPlayer()
{
}

void EntityPlayer::_bind_methods()
{
}

void EntityPlayer::_ready()
{
	_chat_name_label = get_node<Label3D>("%ChatNameLabel");
	_healthbar_mesh = get_node<MeshInstance3D>("%HealthbarMesh");
}

void EntityPlayer::set_chat_name(String name)
{
	_chat_name = name;
	if (is_node_ready()) {
		_chat_name_label->set_text(name);
	}
}

String EntityPlayer::get_chat_name()
{
	return _chat_name;
}

void EntityPlayer::set_health(int value)
{
	_health = value;
	if (is_node_ready()) {
		Ref<QuadMesh> quad_mesh = _healthbar_mesh->get_mesh();
		auto health_ratio = (float) value / DEFAULT_HEALTH;
		auto new_healhbar_width = 0.38f * health_ratio;
		quad_mesh->set_size(Vector2(new_healhbar_width, 0.04f));
		quad_mesh->set_center_offset(Vector3(-0.19f + new_healhbar_width / 2.0f, 0.0f, 0.002f));
	}
}

int EntityPlayer::get_health()
{
	return _health;
}

void EntityPlayer::set_model_variant(String value)
{
	_model_variant = value;
}

String EntityPlayer::get_model_variant()
{
	return _model_variant;
}
