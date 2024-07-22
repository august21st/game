#pragma once
#include <godot_cpp/classes/area3d.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/node3d.hpp>

using namespace godot;

// Used for ladders, climbable objects, etc.
class ClimbableArea : public Area3D {
	GDCLASS(ClimbableArea, Area3D);

protected:
	Engine* _engine;
	static void _bind_methods();

public:
	ClimbableArea();
	~ClimbableArea();
	void _ready() override;
	void _on_body_entered(Node3D* body);
	void _on_body_exited(Node3D* body);
};
