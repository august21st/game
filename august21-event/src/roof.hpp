#pragma once
#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/classes/area3d.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/animation.hpp>
#include <godot_cpp/classes/animation_player.hpp>
#include <godot_cpp/classes/random_number_generator.hpp>
#include <map>

#include "network_manager.hpp"
#include "board_mesh.hpp"
#include "player_body.hpp"

using namespace std;
using namespace godot;

class Roof : public Node3D {
	GDCLASS(Roof, Node3D);

protected:
	Engine* _engine;
	NetworkManager* _network_manager;
	Area3D* _floor_area;
	AnimationPlayer* _sky_animation_player;
	BoardMesh* _board_mesh;
	Ref<RandomNumberGenerator> _random;
	PlayerBody* _player;
	static void _bind_methods();
	const String _death_titles[6] = {
		"FATALITY",
		"YOU PERISHED...",
		"IT'S OVER!",
		"SKILL ISSUE",
		"R.I.P",
		"DEAD, LOL"
	};
	const String _death_messages[6] = {
		"[center][fade start=4 length=24]Death is not an option. But..[/fade][/center]",
		"\n[center][pulse freq=1.0 color=#ffffff40 ease=-2.0]Want to play?[/pulse][/center]",
		"\n[center][tornado radius=10.0 freq=1.0 connected=1]How was the fall?[/tornado][/center]",
		"\n[center][wave amp=50.0 freq=1.0 connected=1][color=red]You can kill the man but not the idea...[/color][/wave][/center]",
		"\n[center][rainbow freq=1.0 sat=0.8 val=0.8]🪑 is 🔥, 🕳️ is 🔥, and you too[/rainbow][/center]",
		"\n[center]Peace never was an opinion...\n- Goose from Untitled Goose Game[/center]"
	};
	map<int, Node*> _entities;

public:
	Roof();
	~Roof();
	void _ready() override;
	void _process(double delta) override;
	void _on_floor_area_body_entered(Node3D* body);
};
