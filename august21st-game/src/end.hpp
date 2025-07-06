#pragma once
#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/classes/directional_light3d.hpp>
#include <godot_cpp/classes/world_environment.hpp>

#include "phase_scene.hpp"

using namespace std;
using namespace godot;

class End : public PhaseScene {
	GDCLASS(End, PhaseScene)

private:
	DirectionalLight3D* _sun_light;
	WorldEnvironment* _world_environment;
	void _on_graphics_quality_changed(int level);

protected:
	static void _bind_methods();

public:
	End();
	~End();
	void _ready() override;
	void spawn_player(EntityPlayerBase* player) override;
	void run_phase_event(String phase_event) override;
};
