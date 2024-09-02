#include <godot_cpp/classes/node3d.hpp>

using namespace godot;

#include "server.hpp"
#include "chair_gun.hpp"
#include "entity_item_base.hpp"

void ChairGun::_bind_methods()
{

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
	// TODO: Hacky way to get server and current scene, but should work
	auto server = (Server*) get_parent();
	if (server == nullptr) {
		UtilityFunctions::print("Couldn't run serverside use: server autoload was null");
		return;
	}
	auto phase_scene = server->get_current_phase_scene();
}
