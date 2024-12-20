#pragma once
#include <godot_cpp/variant/vector3.hpp>
#include <godot_cpp/classes/character_body3d.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/label3d.hpp>
#include <godot_cpp/classes/mesh_instance3d.hpp>

// Forward declare EntityItemBase as it is only used as a pointer here
class EntityItemBase;

using namespace godot;

/**
 * @brief Base class for all player-like entities, including the first person playermodel, 
 * other clientside players, and the serverside player entity representations.
 */
class EntityPlayerBase : public CharacterBody3D {
	GDCLASS(EntityPlayerBase, CharacterBody3D);

protected:
	const int DEFAULT_HEALTH = 100;
	const int MAX_INVENTORY_SIZE = 3;
	int _id;
	String _chat_name;
	int _health;
	String _model_variant;
	List<EntityItemBase*> _inventory;
	int _inventory_current;

protected:
	static void _bind_methods();

public:
	EntityPlayerBase();
	~EntityPlayerBase();
	virtual void set_id(int id);
	virtual int get_id();
	virtual void set_chat_name(String name);
	virtual String get_chat_name();
	virtual void set_health(int value);
	virtual int get_health();
	virtual void set_model_variant(String value);
	virtual String get_model_variant();
	virtual void set_inventory(List<EntityItemBase*> value);
	virtual List<EntityItemBase*>* get_inventory();
	virtual void set_inventory_current(int value);
	virtual int get_inventory_current();
	virtual void respawn(String phase_scene, Vector3 position = Vector3(0, 0, 0));
	virtual void die(String reason="", String message="");
};
