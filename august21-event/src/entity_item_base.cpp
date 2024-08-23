#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/classes/area3d.hpp>
#include <godot_cpp/variant/node_path.hpp>
#include <godot_cpp/classes/rigid_body3d.hpp>
#include <godot_cpp/classes/global_constants.hpp>
#include  "node_shared.hpp"

#include "entity_item_base.hpp"

using namespace NodeShared;

EntityItemBase::EntityItemBase() : _can_grab_override(-1), _grab_area(nullptr), _grab_area_path(NodePath()),
	_item_node(nullptr), _item_node_path(NodePath()), /* _handle(nullptr), _handle_path(NodePath()), */
	_previous_parent(nullptr), _grabbed(false)
{
}

EntityItemBase::~EntityItemBase()
{
}

void EntityItemBase::_bind_methods()
{
	ClassDB::bind_method(D_METHOD("get_can_grab_override"), &EntityItemBase::get_can_grab_override);
	ClassDB::bind_method(D_METHOD("set_can_grab_override", "value"), &EntityItemBase::set_can_grab_override);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "can_grab_override"), "set_can_grab_override", "get_can_grab_override");

	ClassDB::bind_method(D_METHOD("get_grab_area_path"), &EntityItemBase::get_grab_area_path);
	ClassDB::bind_method(D_METHOD("set_grab_area_path", "path"), &EntityItemBase::set_grab_area_path);
	ADD_PROPERTY(PropertyInfo(Variant::NODE_PATH, "grab_area_path", PROPERTY_HINT_NODE_PATH_VALID_TYPES, "Area3D"),
		"set_grab_area_path", "get_grab_area_path");
	ClassDB::bind_method(D_METHOD("get_grab_area"), &EntityItemBase::get_grab_area);
	ClassDB::bind_method(D_METHOD("set_grab_area", "value"), &EntityItemBase::set_grab_area);
	/*ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "grab_area"),
	"set_grab_area", "get_grab_area");*/

	ClassDB::bind_method(D_METHOD("get_item_name"), &EntityItemBase::get_item_name);
	ClassDB::bind_method(D_METHOD("set_item_name", "value"), &EntityItemBase::set_item_name);
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "item_name"),
		"set_item_name", "get_item_name");

	ClassDB::bind_method(D_METHOD("get_item_node_path"), &EntityItemBase::get_item_node_path);
	ClassDB::bind_method(D_METHOD("set_item_node_path",  "path"), &EntityItemBase::set_item_node_path);
	ADD_PROPERTY(PropertyInfo(Variant::NODE_PATH, "item_node_path", PROPERTY_HINT_NODE_PATH_VALID_TYPES, "Node3D"),
		"set_item_node_path", "get_item_node_path");
	ClassDB::bind_method(D_METHOD("get_item_node"), &EntityItemBase::get_item_node);
	ClassDB::bind_method(D_METHOD("set_item_node", "value"), &EntityItemBase::set_item_node);
	/*ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "item_node"),
		"set_item_node", "get_item_node");*/

	/*ClassDB::bind_method(D_METHOD("get_handle_path"), &EntityItemBase::get_handle_path);
	ClassDB::bind_method(D_METHOD("set_handle_path", "path"), &EntityItemBase::set_handle_path);
	ADD_PROPERTY(PropertyInfo(Variant::NODE_PATH, "handle_path", PROPERTY_HINT_NODE_PATH_VALID_TYPES, "Node3D"),
		"set_handle_path", "get_handle_path");
	ClassDB::bind_method(D_METHOD("get_handle"), &EntityItemBase::get_handle);
	ClassDB::bind_method(D_METHOD("set_handle", "value"), &EntityItemBase::set_handle);
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "handle"),
		"set_handle", "get_handle");*/

	ClassDB::bind_method(D_METHOD("get_grabbed"), &EntityItemBase::get_grabbed);
	ClassDB::bind_method(D_METHOD("set_grabbed", "value"), &EntityItemBase::set_grabbed);
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "grabbed"), "set_grabbed", "get_grabbed");

	ClassDB::bind_method(D_METHOD("get_hold_animation"), &EntityItemBase::get_hold_animation);
	ClassDB::bind_method(D_METHOD("set_hold_animation", "value"), &EntityItemBase::set_hold_animation);
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "hold_animation"), "set_hold_animation", "get_hold_animation");

	ClassDB::bind_method(D_METHOD("get_thumbnail_path"), &EntityItemBase::get_thumbnail_path);
	ClassDB::bind_method(D_METHOD("set_thumbnail_path", "value"), &EntityItemBase::set_thumbnail_path);
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "thumbnail_path", PROPERTY_HINT_FILE),
		"set_thumbnail_path", "get_thumbnail_path");
}

void EntityItemBase::_ready()
{
	_can_grab_override = true;
	_previous_parent = get_parent_node_3d();
	set_freeze_mode(RigidBody3D::FREEZE_MODE_KINEMATIC);

	// Handle nodepaths set before _ready (usually from editor)
	// Setting _path and node will both incur eachother, only either needs be set
	if (!_grab_area_path.is_empty()) {
		_grab_area = get_node<Area3D>(_grab_area_path);
	}
	else if (_grab_area != nullptr) {
		_grab_area_path  = _grab_area->get_path();
	}
	if (!_item_node_path.is_empty()) {
		_item_node = get_node<Node3D>(_item_node_path);
	}
	else if (_item_node != nullptr) {
		_item_node_path = _item_node->get_path();
	}
	/*if (!_handle_path.is_empty()) {
		_handle = get_node<Node3D>(_handle_path);
	}
	else if (_handle != nullptr) {
		_handle_path = _handle->get_path();
	}*/
}

void EntityItemBase::set_can_grab_override(int value)
{
	_can_grab_override = value;
}

int EntityItemBase::get_can_grab_override()
{
	return _can_grab_override;
}

void EntityItemBase::set_grab_area_path(const NodePath& path)
{
	_grab_area_path = path;
	if (is_node_ready() && !path.is_empty()) {
		_grab_area = get_node<Area3D>(path);
	}
}

NodePath EntityItemBase::get_grab_area_path()
{
	return _grab_area_path;
}

void EntityItemBase::set_grab_area(Area3D* value)
{
	_grab_area = value;
	_grab_area_path = value->get_path();
}

Area3D* EntityItemBase::get_grab_area()
{
	return _grab_area;
}

void EntityItemBase::set_item_name(String value)
{
	_item_name = value;
}

String EntityItemBase::get_item_name()
{
	return _item_name;
}

void EntityItemBase::set_item_node_path(const NodePath& path)
{
	_item_node_path = path;
	if (is_node_ready() && !path.is_empty()) {
		_item_node = get_node<Node3D>(path);
	}
}

NodePath EntityItemBase::get_item_node_path()
{
	return _item_node_path;
}

void EntityItemBase::set_item_node(Node3D* value)
{
	_item_node = value;
	_item_node_path = value->get_path();
}

Node3D* EntityItemBase::get_item_node()
{
	return _item_node;
}

/*void EntityItemBase::set_handle_path(const NodePath& path)
{
	_handle_path  = path;
	if (is_node_ready() && !path.is_empty()) {
		_handle = get_node<Node3D>(path);
	}
}

NodePath EntityItemBase::get_handle_path()
{
	return _handle_path;
}

void EntityItemBase::set_handle(Node3D* value)
{
	_handle = value;
	_handle_path = _handle->get_path();
}

Node3D* EntityItemBase::get_handle()
{
	return _handle;
}*/

void EntityItemBase::set_grabbed(bool value)
{
	_grabbed = value;
}

bool EntityItemBase::get_grabbed()
{
	return _grabbed;
}

void  EntityItemBase::set_hold_animation(String value)
{
	_hold_animation = value;
}

String EntityItemBase::get_hold_animation()
{
	return _hold_animation;
}

void EntityItemBase::set_thumbnail_path(String value)
{
	_thumbnail_path = value;
}

String EntityItemBase::get_thumbnail_path()
{
	return _thumbnail_path;
}

// Client methods
bool EntityItemBase::can_grab()
{
	if (_can_grab_override != -1) {
		return (bool) _can_grab_override;
	}

	// We are already grabbed (by something else), refuse
	return !_grabbed;
}

bool EntityItemBase::try_grab()
{
	if (!can_grab()) {
		return false;
	}

	_previous_parent = get_parent_node_3d();
	orphan_node(this);
	set_freeze_enabled(true);
	_grabbed = true;
	return true;
}

void EntityItemBase::drop()
{
	orphan_node(this);
	if (_previous_parent != nullptr) {
		_previous_parent->add_child(this);
	}
	set_freeze_enabled(false);
	_grabbed = false;
}
