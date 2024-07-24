#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/variant/signal.hpp>
#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/classes/random_number_generator.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/classes/area3d.hpp>
#include <godot_cpp/classes/animation.hpp>
#include <godot_cpp/classes/animation_player.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/callable.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/method_bind.hpp>
#include <dataproto_cpp/dataproto.hpp>

#include "godot_cpp/templates/hash_map.hpp"
#include "network_manager.hpp"
#include "network_shared.hpp"
#include "board_mesh.hpp"
#include "roof.hpp"

using namespace godot;
using namespace dataproto;
using namespace NetworkShared;

Roof::Roof() : _network_manager(nullptr)
{
}

Roof::~Roof()
{
}

void Roof::_bind_methods()
{
	ClassDB::bind_method(D_METHOD("_on_floor_area_body_entered", "body"),
		&Roof::_on_floor_area_body_entered);
}

void Roof::_ready()
{
	_engine = Engine::get_singleton();
	if (_engine->is_editor_hint()) {
		return;
	}

	_network_manager = (NetworkManager*) _engine->get_singleton("NetworkManager");
	if (_network_manager == nullptr) {
		UtilityFunctions::printerr("Could not initialise network manager: singleton was null");
	}

	_floor_area = get_node<Area3D>("%FloorArea");
	_floor_area->connect("body_entered", Callable(this, "_on_floor_area_body_entered"));

	_player = get_node<PlayerBody>("%PlayerBody");

	_sky_animation_player = get_node<AnimationPlayer>("%SkyAnimationPlayer");
	_sky_animation_player->set_current_animation("roof_sky_animation");

	auto roof_sky_animation = _sky_animation_player->get_animation("roof_sky_animation");
	roof_sky_animation->set_loop_mode(Animation::LoopMode::LOOP_LINEAR);
	_sky_animation_player->play();

	_board_mesh = get_node<BoardMesh>("%BoardMesh");
	_board_mesh->load_canvas();

	_random = Ref<RandomNumberGenerator>();
	_random.instantiate();

	_entities = { };
}

void Roof::_on_floor_area_body_entered(Node3D* body)
{
	if (body == _player) {
		auto death_title_i = _random->randi_range(0, size(_death_titles) - 1);
		auto death_message_i = _random->randi_range(0, size(_death_messages) - 1);
		_player->die(_death_titles[death_title_i], _death_messages[death_message_i]);
		UtilityFunctions::print("roof: Player killed (fell out of map bounds)");
	}
}

void Roof::_process(double delta)
{
	if (_engine->is_editor_hint()) {
		return;
	}

	if (_network_manager != nullptr) {
		auto packets = _network_manager->poll_next_packets();
		for (BufReader packet : packets) {
			uint8_t code = packet.u8();
			switch (code) {
				case ServerPacket::ENTITY_CREATE: {
					auto id = packet.u32();
					auto type_str = packet.str().copy();
					auto type = String(type_str);
					std::free(type_str);
					break;
				}
				case ServerPacket::ENTITY_UPDATE: {
					auto id = packet.u32();
					if (_entities[id] == nullptr) {
						UtilityFunctions::print("roof: Couldn't update entity with id {0}, entity not found.",
							Array::make(id));
						break;
					}
					auto property_str = packet.str().copy();
					auto property = String(property_str);
					std::free(property_str);
					break;
				}
				case ServerPacket::ENTITY_DELETE: {
					auto id = packet.u32();
					if (_entities[id] == nullptr) {
						UtilityFunctions::print("roof: Couldn't delete entity with id {0}, entity not found.",
							Array::make(id));
						break;
					}
					break;
				}
			}
		}
	}
}
