#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/classes/packed_scene.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/world_environment.hpp>
#include <godot_cpp/classes/environment.hpp>
#include <godot_cpp/variant/typed_array.hpp>
#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/classes/window.hpp>
#include <dataproto_cpp/dataproto.hpp>

#include "node_shared.hpp"
#include "game_root.hpp"
#include "server.hpp"
#include "client.hpp"

using namespace godot;
using namespace dataproto;

namespace NodeShared {
	Error load_scene(String scene_path, Node** out_scene_instance)
	{
		auto resource_loader = ResourceLoader::get_singleton();
		auto scene_resource = resource_loader->load(scene_path);
		if (!scene_resource.is_valid() || !scene_resource->is_class("PackedScene")) {
			UtilityFunctions::printerr("Failed to load scene: file not found");
			return Error::ERR_FILE_NOT_FOUND;
		}

		Ref<PackedScene> packed_scene = scene_resource;
		if (!packed_scene.is_valid() || !packed_scene->can_instantiate()) {
			UtilityFunctions::printerr("Failed to load scene: resource was invalid");
			return Error::ERR_INVALID_DATA;
		}

		auto scene_instance = packed_scene->instantiate();
		*out_scene_instance = scene_instance;
		return Error::OK;
	}

	void orphan_node(Node* node)
	{
		auto node_parent = node->get_parent();
		if (node_parent != nullptr) {
			node_parent->remove_child(node);
		}
	}

	int node_children_count_recursive(Node* node, int count)
	{
		auto children = node->get_children();
		for (auto i = 0; i < node->get_child_count(); i++) {
			count += node_children_count_recursive(node->get_child(i), count);
		}
		return count;
	}

	Vector2 circular_clamp(const Vector2& vector, const Vector2& min, const Vector2& max)
	{
		Vector2 centre = (min + max) * 0.5f;
		float radius = MIN((max.x - min.x) * 0.5f, (max.y - min.y) * 0.5f);
		Vector2 to_vector = vector - centre;
		float length = to_vector.length();

		if (length > radius) {
			return centre + to_vector.normalized() * radius;
		}

		return vector;
	}

	void set_environment(WorldEnvironment* environment_node, String environment_path)
	{
		auto resource_loader = ResourceLoader::get_singleton();
		auto environment_resource = resource_loader->load(environment_path);
		if (environment_resource.is_valid() && environment_resource->is_class("Environment")) {
			Ref<Environment> environment = environment_resource;
			if (!environment.is_valid()) {
				UtilityFunctions::printerr("Failed to load enviromnent: resource was invalid");
			}
			else {
				environment_node->set_environment(environment);
			}
		}
		else {
			UtilityFunctions::printerr("Failed to load environment: file not found");
		}
	}

	Server* get_global_server(Node* origin)
	{
		auto server = origin->get_tree()->get_root()->get_node<Server>("/root/Server");
		if (server == nullptr) {
			return nullptr;
		}
		return server;
	}

	Client* get_global_client(Node* origin)
	{
		if (!origin->get_tree()->get_root()->has_node("/root/Client")) {
			return nullptr;
		}

		auto client = origin->get_tree()->get_root()->get_node<Client>("/root/Client");
		if (client == nullptr) {
			return nullptr;
		}
		return client;
	}

	GameRoot* get_game_root(Node* origin)
	{
		auto root = origin->get_tree()->get_root()->get_child(0);
		auto game_root = Object::cast_to<GameRoot>(root);
		if (game_root == nullptr) {
			return nullptr;
		}
		return game_root;
	}
}
