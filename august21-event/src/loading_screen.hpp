#pragma once
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/classes/web_socket_peer.hpp>
#include <godot_cpp/classes/resource_loader.hpp>

#include "client.hpp"

using namespace godot;

class LoadingScreen : public Node3D {
	GDCLASS(LoadingScreen, Node3D);

protected:
	Engine* _engine;
	ResourceLoader* _resource_loader;
	Client* _client;
	Label* _players_label;
	Node3D* _tube;
	static void _bind_methods();

public:
	LoadingScreen();
	~LoadingScreen();
	void _ready() override;
	void _on_packet_received(PackedByteArray packed_packet);
	void _on_graphics_quality_changed(int level);
};
