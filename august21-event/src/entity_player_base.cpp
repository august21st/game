#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/label3d.hpp>
#include <godot_cpp/classes/mesh_instance3d.hpp>

using namespace godot;

#include "entity_player_base.hpp"

EntityPlayerBase::EntityPlayerBase()
{
	_health = DEFAULT_HEALTH;
	_chat_name = "";
}

EntityPlayerBase::~EntityPlayerBase()
{
}

void EntityPlayerBase::_bind_methods()
{
	ClassDB::bind_method(D_METHOD("get_chat_name"), &EntityPlayerBase::get_chat_name);
	ClassDB::bind_method(D_METHOD("set_chat_name"), &EntityPlayerBase::set_chat_name);
	ClassDB::bind_method(D_METHOD("get_health"), &EntityPlayerBase::get_health);
	ClassDB::bind_method(D_METHOD("set_health"), &EntityPlayerBase::set_health);
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "chat_name"), "set_chat_name", "get_chat_name");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "health"), "set_health", "get_health");
}

void EntityPlayerBase::set_chat_name(String name)
{
	_chat_name = name;
}

String EntityPlayerBase::get_chat_name()
{
	return _chat_name;
}

void EntityPlayerBase::set_health(int value)
{
	_health = value;
}

int EntityPlayerBase::get_health()
{
	return _health;
}
