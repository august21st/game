#pragma once
#include <godot_cpp/classes/node3d.hpp>

using namespace godot;

class ChairGun : public Node3D {
	GDCLASS(ChairGun, Node3D);

protected:
	static void _bind_methods();

public:
	ChairGun();
	~ChairGun();
	bool is_item();
	void server_use();
};
