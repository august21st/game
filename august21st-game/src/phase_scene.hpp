#pragma once
#include <godot_cpp/classes/node3d.hpp>

#include "game_root.hpp"
class EntityPlayerBase;

using namespace std;
using namespace godot;

class PhaseScene : public Node3D {
	GDCLASS(PhaseScene, Node3D);

protected:
	GameRoot* _game_root;
	static void _bind_methods();

public:
	PhaseScene();
	~PhaseScene();
	virtual void spawn_player(EntityPlayerBase* player);
	virtual void run_phase_event(String phase_event);
};
