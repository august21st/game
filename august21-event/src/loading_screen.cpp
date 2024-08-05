#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/variant/signal.hpp>
#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/classes/web_socket_peer.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/classes/window.hpp>
#include <dataproto_cpp/dataproto.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/classes/mesh_instance3d.hpp>
#include <godot_cpp/variant/callable.hpp>
#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/classes/material.hpp>

#include "client.hpp"
#include "loading_screen.hpp"
#include "network_shared.hpp"

using namespace std;
using namespace godot;
using namespace dataproto;
using namespace NetworkShared;

LoadingScreen::LoadingScreen()
{
}

LoadingScreen::~LoadingScreen()
{
}

void LoadingScreen::_bind_methods()
{
	ClassDB::bind_method(D_METHOD("_on_packet_received", "packed_packet"),
		&LoadingScreen::_on_packet_received);
	ClassDB::bind_method(D_METHOD("_on_graphics_quality_changed", "level"),
		&LoadingScreen::_on_graphics_quality_changed);
}

void LoadingScreen::_ready()
{
	_engine = Engine::get_singleton();
	if (_engine && _engine->is_editor_hint()) {
		return;
	}

	_resource_loader = ResourceLoader::get_singleton();

	_client = get_tree()->get_root()->get_node<Client>("/root/GlobalClient");
	if (_client == nullptr) {
		UtilityFunctions::printerr("Could not get client: autoload singleton was null");
		return;
	}
	_client->connect("packet_received", Callable(this, "_on_packet_received"));
	_client->connect("graphics_quality_changed", Callable(this, "_on_graphics_quality_changed"));

	_players_label = get_node<Label>("%PlayerCountLabel");

	_tube = get_node<Node3D>("%Tube");
}

void LoadingScreen::_on_packet_received(PackedByteArray packed_packet)
{
	auto packet = BufReader((char*) packed_packet.ptr(), packed_packet.size());
	uint8_t code = packet.u8();
	switch (code) {
		case ServerPacket::GAME_INFO: {
			auto player_count = packet.u32();
			auto formatted_count = String("Players waiting: {0}")
				.format(Array::make(player_count));
			_players_label->set_text(formatted_count);
			break;
		}
	}
}

void LoadingScreen::_on_graphics_quality_changed(int level)
{
	auto tube_mesh_node = _tube->get_child(0);
	if (tube_mesh_node != nullptr && tube_mesh_node->is_class("MeshInstance3D")) {
		auto tube_mesh = Object::cast_to<MeshInstance3D>(tube_mesh_node);
		auto tube_material_resource = _resource_loader->load(level == 0
			? "res://assets/tube_material_low.tres"
			: "res://assets/tube_material_high.tres");
		if (tube_material_resource.is_valid()) {
			const Ref<Material> tube_material = tube_material_resource;
			if (!tube_material.is_valid()) {
				UtilityFunctions::printerr("Failed to load tube material: resource was invalid");
			}
			else {
				tube_mesh->set_material_override(tube_material);
			}
		}
		else {
			UtilityFunctions::printerr("Failed to load tube material: file not found");
		}
	}

}
