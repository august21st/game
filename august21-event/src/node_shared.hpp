#pragma once
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/world_environment.hpp>
#include <dataproto_cpp/dataproto.hpp>

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

	// Serialised properties / can either be defined as a pointer to the filesystem,
	// or as an inlinne encoded variant. The same case for resource-type properties
	// on an entity.
	enum ObjectType {
		INLINE_NODE,
		FILESYSTEM_NODE,
		INLINE_RESOURCE,
		FILESYSTEM_RESOURCE,
		COMPRESSED_VARIANT,
		VARIANT
	};
	void write_compressed_variant(const Variant& variant, BufWriter& buffer);
	Variant read_compressed_variant(BufReader& buffer);
	void write_variant(const Variant& variant, BufWriter& buffer);
	Variant read_variant(BufReader& buffer);
	void write_entity_data(Node* node, BufWriter& buffer);
	void write_entity_data(String node_path, BufWriter& buffer);
	Node* read_entity_data(BufReader& buffer);
	void set_environment(WorldEnvironment* environment_node, String path);
}
