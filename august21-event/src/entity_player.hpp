#pragma once
#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/classes/engine.hpp>

using namespace godot;

class EntityPlayer : public Node3D {
	GDCLASS(EntityPlayer, Node3D);

private:
	Engine* _engine;
	Client* _client;
	String _chat_name;

protected:
	static void _bind_methods();

public:
	EntityPlayer();
	~EntityPlayer();
	void _ready() override;
	void set_chat_name(String name);
	String get_chat_name();
};
