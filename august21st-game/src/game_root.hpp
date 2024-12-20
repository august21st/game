#pragma once
#include <godot_cpp/classes/node.hpp>

using namespace godot;

class GameRoot : public Node {
	GDCLASS(GameRoot, Node)

protected:
	static void _bind_methods();

public:
	GameRoot();
	~GameRoot();
	virtual String get_current_phase_scene();
	virtual String get_current_phase_event();
	virtual bool has_phase_scene(String name);
	virtual Node* get_phase_scene(String name);
	virtual bool is_server();
	virtual bool is_client();
};
