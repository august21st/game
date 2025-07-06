#pragma once
#include <godot_cpp/classes/node.hpp>

using namespace godot;

/**
 * @brief Startup scene of game - ushers in root Client / Server depending
 * on game startup arguments. The objective of this Node is to ultimately
 * initialise the game and GTFO as fast as possible.
 */
class Loader : public Node {
	GDCLASS(Loader, Node)

protected:
	static void _bind_methods();

public:
	Loader();
	~Loader();
	void _ready() override;
};
