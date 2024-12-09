#include <godot_cpp/templates/list.hpp>
#include <godot_cpp/classes/node.hpp>

#include "entity_info.hpp"

EntityInfo::EntityInfo()
{
}

EntityInfo::EntityInfo(int id, Node* entity, String parent_scene)
{
	_id = id;
	_entity = entity;
	_parent_scene = parent_scene;
	_tracked_properties = { };
}

EntityInfo::~EntityInfo()
{
	if (_entity != nullptr) {
		_entity->queue_free();
	}
}

void EntityInfo::_bind_methods()
{
}

Node* EntityInfo::get_entity()
{
	return _entity;
}

String EntityInfo::get_parent_scene()
{
	return _parent_scene;
}

int EntityInfo::get_id()
{
	return _id;
}

List<String> EntityInfo::get_tracked_properties()
{
	auto property_list = List<String>();
	for (auto &[property, _] : _tracked_properties) {
		property_list.push_back(property);
	}

	return property_list;
}

void EntityInfo::track_property(String property)
{
	_tracked_properties.insert(property, _entity->get(property));
}

void EntityInfo::untrack_property(String property)
{
	_tracked_properties.erase(property);
}

Variant EntityInfo::get_property_value(String property)
{
	auto current_value = _entity->get(property);
	return current_value;
}

bool EntityInfo::tracked_property_changed(String property)
{
	auto current_value = _entity->get(property);
	auto tracked_value = _tracked_properties.get(property);
	_tracked_properties[property] = current_value;
	return current_value != tracked_value;
}
