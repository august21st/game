#include <godot_cpp/core/object.hpp>
#include <godot_cpp/classes/node3d.hpp>

#include "chair_gun.hpp"
#include "entity_item_base.hpp"
#include "entity_info.hpp"
#include "node_shared.hpp"

using namespace godot;
using namespace NodeShared;


void ChairGun::_bind_methods()
{
	ClassDB::bind_method(D_METHOD("server_use"), &ChairGun::server_use);
}

ChairGun::ChairGun()
{
}

ChairGun::~ChairGun()
{
}

bool ChairGun::is_item()
{
	return Object::cast_to<EntityItemBase>(get_parent()) != nullptr;
}

void ChairGun::server_use()
{
	auto server = get_global_server(this);
	if (server == nullptr) {
		UtilityFunctions::print("Couldn't run serverside use: server autoload was null");
		return;
	}
	auto current_scene = server->get_current_phase_scene();
	auto info = server->create_entity("res://scenes/wooden_chair.tscn", current_scene);
	info->track_property("position");
	info->track_property("rotation");

	auto chair = Object::cast_to<RigidBody3D>(info->get_entity());
	if (chair != nullptr) {
		auto global_pos = get_global_position();
		auto gun_forward = Vector3(0.0f, 0.0f, 1.0f);
		auto rotation = get_global_rotation();
		gun_forward.rotate(Vector3(1, 0, 0), rotation.x);
		gun_forward.rotate(Vector3(0, 1, 0), rotation.y);
		gun_forward.rotate(Vector3(0, 0, 1), rotation.z);
		chair->set_position(global_pos + gun_forward);
		chair->apply_impulse(gun_forward * SHOOT_IMPULSE);
	}
}
