#pragma once
#include <godot_cpp/classes/area3d.hpp>
#include <godot_cpp/variant/node_path.hpp>
#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/classes/rigid_body3d.hpp>

using namespace godot;

// An entity:item, as in, an entity of type item, is an entity that represents
// the itemised form of a node, and is usually the direct parent of the node
// it represents. This is the only type that can be 'held' by a PlayerEntityBase.
class EntityItemBase : public RigidBody3D {
	GDCLASS(EntityItemBase, RigidBody3D);

private:
	int _can_grab_override;
	Area3D* _grab_area;
	NodePath _grab_area_path;
	String _item_name;
	Node3D* _item_node;
	NodePath _item_node_path;
	bool _grabbed;
	Node3D* _previous_parent;
	/*Node3D* _handle;
	NodePath _handle_path;*/
	//  Editor annd server methods
	virtual void set_can_grab_override(int value);
	virtual int get_can_grab_override();
	virtual void set_grab_area_path(const NodePath& path);
	virtual NodePath get_grab_area_path();
	virtual void set_grab_area(Area3D* value);
	virtual Area3D* get_grab_area();
	virtual void set_item_name(String value);
	virtual String get_item_name();
	virtual void set_item_node_path(const NodePath& path);
	virtual NodePath get_item_node_path();
	virtual void set_item_node(Node3D* value);
	virtual Node3D* get_item_node();
	virtual void set_grabbed(bool value);
	virtual bool get_grabbed();
	/*virtual void set_handle_path(const NodePath& path);
	virtual NodePath get_handle_path();
	virtual void set_handle(Node3D* value);
	virtual Node3D* get_handle();*/

protected:
	static void _bind_methods();

public:
	EntityItemBase();
	~EntityItemBase();
	void _ready() override;
	// Client methods
	virtual bool can_grab();
	virtual bool try_grab();
	virtual void drop();
};
