#pragma once
#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/classes/area3d.hpp>
#include <godot_cpp/classes/animation.hpp>
#include <godot_cpp/classes/animation_player.hpp>
#include <godot_cpp/classes/random_number_generator.hpp>
#include <godot_cpp/classes/directional_light3d.hpp>
#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/classes/world_environment.hpp>
#include <godot_cpp/classes/texture_rect.hpp>
#include <godot_cpp/classes/h_box_container.hpp>
#include <godot_cpp/classes/rich_text_label.hpp>

#include "client.hpp"
#include "server.hpp"
#include "board_mesh.hpp"
#include "player_body.hpp"

using namespace std;
using namespace godot;

class Roof : public Node3D {
	GDCLASS(Roof, Node3D);

private:
	// May be null
	Client* _client;
	// May be null
	PlayerBody* _player;
	// May be null
	Server* _server;
	ResourceLoader* _resource_loader;
	DirectionalLight3D* _sun_light;
	Area3D* _floor_area;
	AnimationPlayer* _sky_animation_player;
	WorldEnvironment* _world_environment;
	BoardMesh* _board_mesh;
	Node3D* _player_spawnpoint;
	HBoxContainer* _event_container;
	RichTextLabel* _event_title_label;
	TextureRect* _event_image;
	Ref<RandomNumberGenerator> _random;
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
	void _on_floor_area_body_entered(Node3D* body);
	void _on_graphics_quality_changed(int level);
	void server_run_phase_event(String phase_event);

protected:
	static void _bind_methods();

public:
	Roof();
	~Roof();
	void _ready() override;
	void spawn_player(PlayerBody* player);
	void run_phase_event(String phase_event);
};
