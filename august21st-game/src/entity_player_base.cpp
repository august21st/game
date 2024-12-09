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
	ClassDB::bind_method(D_METHOD("get_model_variant"), &EntityPlayerBase::get_model_variant);
	ClassDB::bind_method(D_METHOD("set_model_variant"), &EntityPlayerBase::set_model_variant);
	//ClassDB::bind_method(D_METHOD("get_inventory"), &EntityPlayerBase::get_inventory);
	//ClassDB::bind_method(D_METHOD("set_inventory"), &EntityPlayerBase::set_inventory);
	ClassDB::bind_method(D_METHOD("get_inventory_current"), &EntityPlayerBase::get_inventory_current);
	ClassDB::bind_method(D_METHOD("set_inventory_current"), &EntityPlayerBase::set_inventory_current);
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "chat_name"), "set_chat_name", "get_chat_name");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "health"), "set_health", "get_health");
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "model_variant"), "set_model_variant", "get_model_variant");
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

void EntityPlayerBase::set_model_variant(String value)
{
	_model_variant = value;
}

String EntityPlayerBase::get_model_variant()
{
	return _model_variant;
}

List<EntityItemBase*>* EntityPlayerBase::get_inventory()
{
	return &_inventory;
}

void EntityPlayerBase::set_inventory(List<EntityItemBase*> value)
{
	_inventory = value;
}

void EntityPlayerBase::set_inventory_current(int value)
{
	_inventory_current = value;
}

int EntityPlayerBase::get_inventory_current()
{
	return _inventory_current;
}

void EntityPlayerBase::respawn(Vector3 position)
{
    set_global_position(position);
    set_health(DEFAULT_HEALTH);
}
