#pragma once
#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/directional_light3d.hpp>
#include <godot_cpp/classes/world_environment.hpp>

#include "client.hpp"

using namespace std;
using namespace godot;

class End : public Node3D {
	GDCLASS(End, Node3D);

private:
	Engine* _engine;
	Client* _client;
	DirectionalLight3D* _sun_light;
	WorldEnvironment* _world_environment;
	void _on_graphics_quality_changed(int level);
	void _server_run_phase_event(String phase_event);

protected:
	static void _bind_methods();

public:
	End();
	~End();
	void _ready() override;
	void spawn_player(PlayerBody* player);
	void run_phase_event(String phase_event);
};
