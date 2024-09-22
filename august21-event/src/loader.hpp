#pragma once
#include <godot_cpp/classes/node.hpp>

using namespace godot;

// Startup scene of program - Ushers in root Client / Server depending on game startup
// the objective of this class is to ultimately GTFO as fast as possible
class Loader : public Node {
	GDCLASS(Loader, Node);

protected:
	static void _bind_methods();

public:
	Loader();
	~Loader();
};
