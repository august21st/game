#pragma once
#include <godot_cpp/templates/hash_map.hpp>
#include <godot_cpp/classes/node.hpp>
#include <dataproto_cpp/dataproto.hpp>

using namespace dataproto;
using namespace godot;

class EntityInfo : public Object {
	GDCLASS(EntityInfo, Object);

private:
	int _id;
	Node* _entity;
	String _parent_scene;
	HashMap<String, Variant> _tracked_properties;

public:
	EntityInfo(int id, Node* entity, String parent_scene);
	~EntityInfo();
	int get_id();
	Node* get_entity();
	String get_parent_scene();
	List<String> get_tracked_properties();
	void track_property(String property);
	void untrack_property(String property);
	Variant get_property_value(String property);
	bool tracked_property_changed(String property);
};
