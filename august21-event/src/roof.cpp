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
#include <godot_cpp/templates/hash_map.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/classes/window.hpp>
#include <godot_cpp/classes/directional_light3d.hpp>
#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/classes/environment.hpp>
#include <godot_cpp/classes/camera3d.hpp>
#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/classes/world_environment.hpp>

#include "client.hpp"
#include "board_mesh.hpp"
#include "roof.hpp"

using namespace godot;
using namespace dataproto;

Roof::Roof()
{
}

Roof::~Roof()
{
}

void Roof::_bind_methods()
{
	ClassDB::bind_method(D_METHOD("_on_floor_area_body_entered", "body"),
		&Roof::_on_floor_area_body_entered);
	ClassDB::bind_method(D_METHOD("_on_graphics_quality_changed", "level"),
		&Roof::_on_graphics_quality_changed);
}

void Roof::_ready()
{
	_engine = Engine::get_singleton();
	if (_engine->is_editor_hint()) {
		return;
	}

	_client = get_tree()->get_root()->get_node<Client>("/root/GlobalClient");
	if (_client == nullptr) {
		UtilityFunctions::printerr("Could not get client: autoload singleton was null");
		return;
	}
	_client->connect("graphics_quality_changed", Callable(this, "_on_graphics_quality_changed"));
	_player = _client->get_player_body();

	_resource_loader = ResourceLoader::get_singleton();

	_sun_light = get_node<DirectionalLight3D>("%SunLight");

	_floor_area = get_node<Area3D>("%FloorArea");
	_floor_area->connect("body_entered", Callable(this, "_on_floor_area_body_entered"));

	_sky_animation_player = get_node<AnimationPlayer>("%SkyAnimationPlayer");
	_sky_animation_player->set_current_animation("roof_sky_animation");

	auto roof_sky_animation = _sky_animation_player->get_animation("roof_sky_animation");
	roof_sky_animation->set_loop_mode(Animation::LoopMode::LOOP_LINEAR);
	_sky_animation_player->play();

	_board_mesh = get_node<BoardMesh>("%BoardMesh");
	_board_mesh->load_canvas();

	_random = Ref<RandomNumberGenerator>();
	_random.instantiate();
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

void Roof::_on_graphics_quality_changed(int level)
{
	String environment_path;
	if (level == 0) {
		environment_path = "res://assets/roof_environment_high.tres";
	}
	else if (level == 1) {
		environment_path = "res://assets/roof_environment_low.tres";
	}
	auto environment_resource = _resource_loader->load(environment_path);
	if (environment_resource.is_valid() && environment_resource->is_class("Environment")) {
		const Ref<Environment> environment = environment_resource;
		if (!environment.is_valid()) {
			UtilityFunctions::printerr("Failed to load enviromnent: resource was invalid");
		}
		else {
			_world_environment->set_environment(environment);
		}
	}
	else {
		UtilityFunctions::printerr("Failed to load environment: file not found");
	}

	_sun_light->set_shadow(level == 0 ? false : true);
}
