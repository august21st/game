#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/variant/signal.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/variant/callable.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/method_bind.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/classes/window.hpp>
#include <godot_cpp/classes/directional_light3d.hpp>
#include <godot_cpp/classes/label3d.hpp>
#include <godot_cpp/classes/mesh_instance3d.hpp>
#include <godot_cpp/classes/mesh.hpp>
#include <godot_cpp/classes/quad_mesh.hpp>
#include <godot_cpp/templates/list.hpp>
#include <godot_cpp/variant/packed_byte_array.hpp>
#include <godot_cpp/variant/vector3.hpp>
#include <dataproto_cpp/dataproto.hpp>

#include "server.hpp"
#include "entity_player.hpp"
#include "network_shared.hpp"
#include "node_shared.hpp"

using namespace godot;
using namespace dataproto;
using namespace NetworkShared;
using namespace NodeShared;

EntityPlayer::EntityPlayer() :
	_chat_name_label(nullptr),
	_healthbar_mesh(nullptr),
	_game_root(nullptr)
{
	_current_animation = "idle";
}

EntityPlayer::~EntityPlayer()
{
}

void EntityPlayer::_bind_methods()
{
	ClassDB::bind_method(D_METHOD("_on_packet_received", "sender_id", "packed_packet"),
		&EntityPlayer::_on_packet_received);
}

void EntityPlayer::_ready()
{
	_chat_name_label = get_node<Label3D>("%ChatNameLabel");
	_healthbar_mesh = get_node<MeshInstance3D>("%HealthbarMesh");
	_game_root = get_game_root(this);
	_game_root->connect("packet_received", Callable(this, "_on_packet_received"));
}

void EntityPlayer::_on_packet_received(int sender_id, PackedByteArray packed_packet)
{
	if (_game_root == nullptr || !_game_root->is_server() || sender_id != _id) {
		return;
	}

	auto client = (Client*) _game_root;
	auto packet = BufReader((char*) packed_packet.ptr(), packed_packet.size());
	auto code = static_cast<ClientPacket>(packet.u8());

	switch (code) {
		case ClientPacket::SET_CHAT_NAME: {
			auto chat_name_str = (string) packet.str();
			auto chat_name = String(chat_name_str.c_str());
			set_chat_name(chat_name);
			break;
		}
		case ClientPacket::SET_MODEL_VARIANT: {
			auto value_str = (string) packet.str();
			auto value = String(value_str.c_str());
			set_model_variant(value);
			break;
		}
		case ClientPacket::UPDATE_MOVEMENT: {
			auto position = read_vector3(packet);
			auto velocity = read_vector3(packet);
			auto rotation = read_vector3(packet);
			auto current_animation_str = packet.str();
			update_movement(position, velocity, rotation);
			break;
		}
		case ClientPacket::ACTION_GRAB: {
			auto item_id = packet.u32();
			action_grab(item_id);
			break;
		}
		case ClientPacket::ACTION_TAKE_DAMAGE: {
			auto value = packet.u32();
			take_damage(value);
			break;
		}
		case ClientPacket::ACTION_DIE: {
			die();
			break;
		}
		case ClientPacket::ACTION_RESPAWN: {
			// TODO: Flesh out
			auto scene = _game_root->get_current_phase_scene();
			auto position = Vector3(0, 0, 0);
			respawn(scene, position);
			break;
		}
		case ClientPacket::ACTION_DROP: {
			action_drop();
			break;
		}
		case ClientPacket::ACTION_SWITCH: {
			auto inventory_i = packet.u8();
			action_switch(inventory_i);
			break;
		}
		case ClientPacket::ACTION_USE: {
			action_use();
			break;
		}
		default: {
			break;
		}
	}
}

void EntityPlayer::_physics_process(double delta)
{
	set_velocity(_velocity);
	broadcast_movement();
}

void EntityPlayer::update_movement(Vector3 position, Vector3 velocity, Vector3 rotation)
{
	set_global_position(position);
	set_global_rotation(rotation);
	_velocity = velocity;
}

void EntityPlayer::broadcast_movement()
{
	if (_game_root == nullptr || !_game_root->is_server()) {
		return;
	}

	auto server = (Server*) _game_root;
	auto update_packet = BufWriter();
	update_packet.u8(to_uint8(ServerPacket::UPDATE_MOVEMENT));
	update_packet.u32(_id); // player ID
	auto phase_scene = _game_root->get_current_phase_scene();
	auto phase_scene_str = phase_scene.utf8().get_data();
	update_packet.str(phase_scene_str); // phase scene
	write_vector3(update_packet, get_global_position()); // position
	write_vector3(update_packet, _velocity); // velocity
	write_vector3(update_packet, get_global_rotation()); // rotation
	auto current_animation_str = _current_animation.utf8().get_data();
	update_packet.str(current_animation_str); // current animation
	server->send_to_other_players(_id, update_packet);

}

void EntityPlayer::action_grab(int item_id)
{
	if (_game_root == nullptr || !_game_root->is_server()) {
		return;
	}
	auto server = (Server*) _game_root;
	auto entity_info = server->get_entity(item_id);
	auto item_entity = Object::cast_to<EntityItemBase>(entity_info->get_entity());

	orphan_node(item_entity);
	add_child(item_entity);
	auto inventory = get_inventory();
	inventory->push_back(item_entity);
	set_inventory_current(inventory->size() - 1);

	auto grab_packet = BufWriter();
	grab_packet.u8(to_uint8(ServerPacket::GRAB));
	grab_packet.u32(_id);
	grab_packet.u32(item_id);
	server->send_to_players(grab_packet);
}

void EntityPlayer::action_drop()
{
}

void EntityPlayer::action_use()
{
}

void EntityPlayer::action_switch(int inventory_i)
{
}

void EntityPlayer::take_damage(int value)
{
	set_health(_health - value);
}

void EntityPlayer::set_health(int value)
{
	_health = CLAMP(value, 0, DEFAULT_HEALTH);
	if (is_node_ready()) {
		Ref<QuadMesh> quad_mesh = _healthbar_mesh->get_mesh();
		auto health_ratio = (float) value / DEFAULT_HEALTH;
		auto new_healhbar_width = 0.38f * health_ratio;
		quad_mesh->set_size(Vector2(new_healhbar_width, 0.04f));
		quad_mesh->set_center_offset(Vector3(-0.19f + new_healhbar_width / 2.0f, 0.0f, 0.002f));
	}
	if (_game_root != nullptr) {
		if (_health == 0) {
			die();
		}
	}
	broadcast_health();
}

int EntityPlayer::get_health()
{
	return _health;
}

void EntityPlayer::broadcast_health()
{
	if (_game_root == nullptr || !_game_root->is_server()) {
		return;
	}

	auto server = (Server*) _game_root;
	auto health_packet = BufWriter();
	health_packet.u8(to_uint8(ServerPacket::UPDATE_HEALTH));
	health_packet.u32(_id);
	health_packet.u32(_health);
	server->send_to_players(health_packet);
}


void EntityPlayer::die(String reason, String message)
{
	_health = 0;
	_current_phase_scene = "";
	set_visible(false);
	broadcast_health();
	broadcast_die();
}

void EntityPlayer::broadcast_die()
{
	if (_game_root == nullptr || !_game_root->is_server()) {
		return;
	}

	auto server = (Server*) _game_root;
	auto die_packet = BufWriter();
	die_packet.u8(to_uint8(ServerPacket::DIE));
	die_packet.u32(_id);
	String death_reason = String("");
	die_packet.str(death_reason.utf8().get_data()); //TODO: death_reason, death_message
	String death_message = String("");
	die_packet.str(death_message.utf8().get_data());
	server->send_to_all(die_packet);
}


void EntityPlayer::set_chat_name(String name)
{
	_chat_name = name;
	if (is_node_ready()) {
		_chat_name_label->set_text(name);
	}
}

String EntityPlayer::get_chat_name()
{
	return _chat_name;
}

void EntityPlayer::broadcast_chat_name()
{
	if (_game_root == nullptr) {
		return;
	}
}

void EntityPlayer::set_model_variant(String value)
{
	bool valid = false;
	for (auto variant : _model_variants) {
		if (variant == value) {
			valid = true;
			break;
		}
	}
	if (!valid) {
		UtilityFunctions::printerr("Couldn't set player ",
			_id, "'s model variant: phase scene not found");
		return;
	}

	_model_variant = value;
}

String EntityPlayer::get_model_variant()
{
	return _model_variant;
}

void EntityPlayer::broadcast_model_variant()
{
	if (_game_root == nullptr) {
		return;
	}
}

void EntityPlayer::respawn(String phase_scene, Vector3 position)
{
	if (_game_root == nullptr || !_game_root->is_server()) {
		return;
	}
	auto server = (Server*) _game_root;

	if (!_game_root->has_phase_scene(phase_scene)) {
		UtilityFunctions::printerr("Couldn't respawn player ",
			_id, ": phase scene not found");
		return;
	}

	auto player_scene = _game_root->get_phase_scene(phase_scene);
	if (get_parent() != player_scene) {
		// Teleport serverside client to correct scene
		orphan_node(this);
		player_scene->add_child(this);
	}

	update_movement(position, Vector3(0, 0, 0), Vector3(0, 0, 0));
	set_health(DEFAULT_HEALTH);
	broadcast_respawn();
}

void EntityPlayer::broadcast_respawn()
{
	if (_game_root == nullptr || !_game_root->is_server()) {
		return;
	}

	auto server = (Server*) _game_root;
	auto respawn_packet = BufWriter();
	respawn_packet.u8(to_uint8(ServerPacket::RESPAWN));
	respawn_packet.u32(_id);
	auto phase_scene_str = _current_phase_scene.utf8().get_data();
	respawn_packet.str(phase_scene_str);
	server->send_to_all(respawn_packet);
}
