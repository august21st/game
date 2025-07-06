#pragma once
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/world_environment.hpp>
#include <dataproto_cpp/dataproto.hpp>

#include "server.hpp"
#include "client.hpp"

using namespace godot;
using namespace dataproto;

namespace NodeShared {
	void orphan_node(Node* node);
	Error load_scene(String scene_path, Node** out_scene_instance);
	template<typename T> Error load_scene_strict(String scene_path, T** out_scene_instance)
	{
		Node* scene_instance;
		auto load_error = load_scene(scene_path, &scene_instance);
		if (load_error != Error::OK) {
			return load_error;
		}

		if (scene_instance->get_class() != T::get_class_static()) {
			UtilityFunctions::printerr("Failed to load scene: expected type did not match scene type");
			return godot::Error::ERR_INVALID_DATA;
		}

		*out_scene_instance = (T*) scene_instance;
		return godot::Error::OK;
	}
	int node_children_count_recursive(Node* node, int count = 0);

	/**
	 * @brief Min: top left corner, max: bottom left corner, object can move within a circular area within the rectangle
	 */
	Vector2 circular_clamp(const Vector2& vector, const Vector2& min, const Vector2& max);
	void set_environment(WorldEnvironment* environment_node, String path);

	// TODO: Consider consolidating into GameRoot
	Server* get_global_server(Node* origin);
	Client* get_global_client(Node* origin);
	GameRoot* get_game_root(Node* origin);

	// Constants
	const float PI = 3.14159265358979f;
}
