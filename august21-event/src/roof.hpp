#pragma once
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/node3d.hpp>

using namespace godot;

class Roof : public Node3D {
	GDCLASS(Roof, Node3D);

protected:
	Engine* _engine;
	static void _bind_methods();

public:
	Roof();
	~Roof();
	void _ready() override;
	void _process(double delta) override;
};