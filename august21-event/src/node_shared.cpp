#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/classes/packed_scene.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/world_environment.hpp>
#include <godot_cpp/classes/environment.hpp>
#include <dataproto_cpp/dataproto.hpp>

#include "node_shared.hpp"

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

	void write_entity_data(Node* node, BufWriter* buffer)
	{
		// Write node data
		auto entity_variant = (const Variant&) node;
		auto temp_buffer = PackedByteArray();
		// TODO: Investigate better ways to find encoded variant size beforehand
		auto temp_buffer_reserve = 512;
		auto entity_size = -1;
		while (true) {
			temp_buffer.resize(temp_buffer_reserve);
			entity_size = temp_buffer.encode_var(0, node, true);
			if (entity_size != -1) {
				break;
			}
			temp_buffer_reserve *= 2;
		}

		temp_buffer = temp_buffer.compress(FileAccess::COMPRESSION_FASTLZ);
		buffer->flint(entity_size); // uncompressed entity_size
		buffer->flint(temp_buffer.size()); // compressed entity_data size
		buffer->arr(temp_buffer.ptrw(), temp_buffer.size()); // compressed entity_data

		// Write children
		auto child_count = node->get_child_count();
		buffer->flint(child_count);
		for (auto  i = 0 ; i < child_count; i++) {
			write_entity_data(node->get_child(i), buffer);
		}
	}

	Node* read_entity_data(BufReader* buffer)
	{
		auto entity_size = buffer->flint();
		auto compressed_size = buffer->flint();
		auto entity_data = buffer->arr(compressed_size);
		auto temp_buffer = PackedByteArray();
		temp_buffer.resize(entity_data.size);
		memcpy(temp_buffer.ptrw(), entity_data.data, entity_data.size);
		temp_buffer = temp_buffer.decompress(entity_size, FileAccess::COMPRESSION_FASTLZ);

		auto entity = temp_buffer.decode_var(0, true);
		if (entity.get_type() != Variant::OBJECT) {
			UtilityFunctions::print("Couldn't read entity: decoded variant was not an Object.");
			return nullptr;
		}
		Node* entity_node = Object::cast_to<Node>(entity);
		if (entity_node == nullptr) {
			UtilityFunctions::print("Couldn't read entity: decoded variant was not a Node.");
			return nullptr;
		}

		auto child_count = buffer->flint();
		for (auto i =  0; i < child_count; i++) {
			auto child_node = read_entity_data(buffer);
			if (child_node != nullptr) {
				entity_node->add_child(child_node);
			}
		}

		return entity_node;
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
}
