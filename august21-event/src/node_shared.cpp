#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/classes/packed_scene.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/world_environment.hpp>
#include <godot_cpp/classes/environment.hpp>
#include <godot_cpp/variant/typed_array.hpp>
#include <dataproto_cpp/dataproto.hpp>

#include "node_shared.hpp"
#include "godot_cpp/classes/resource.hpp"
#include "godot_cpp/core/class_db.hpp"
#include "godot_cpp/variant/string.hpp"

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

	void write_compressed_variant(const Variant& variant, BufWriter& buffer)
	{
		auto variant_buffer = PackedByteArray();
		// TODO: Investigate better ways to find encoded variant size beforehand
		auto reserve = 64;
		auto variant_size = -1;
		while (true) {
			variant_buffer.resize(reserve);
			variant_size = variant_buffer.encode_var(0, variant, true);
			if (variant_size != -1) {
				// Trim excess from reserve allocations
				variant_buffer.resize(variant_size);
				break;
			}
			reserve *= 2;
		}
		variant_buffer = variant_buffer.compress(FileAccess::COMPRESSION_FASTLZ);
		buffer.flint(variant_size); // uncompressed buffer (variant_size)
		buffer.flint(variant_buffer.size()); // compressed buffer (compressed_size)
		buffer.arr(variant_buffer.ptrw(), variant_buffer.size()); // compressed variant_data
	}

	Variant read_compressed_variant(BufReader& buffer)
	{
		auto variant_size = buffer.flint();
		auto compressed_size = buffer.flint();
		auto variant_data = buffer.arr(compressed_size);
		auto temp_buffer = PackedByteArray();
		temp_buffer.resize(variant_data.size);
		memcpy(temp_buffer.ptrw(), variant_data.data, variant_data.size);
		// TODO: For some reason, the variant fails to read if the decompressed size is variant size, even though
		// variant_size should be the size of the original object. TODO: Investiagate!
		temp_buffer = temp_buffer.decompress(variant_size * 2 /* DEBUG: This <- */, FileAccess::COMPRESSION_FASTLZ);

		auto variant = temp_buffer.decode_var(0, true);
		return variant;
	}

	void write_variant(const Variant& variant, BufWriter& buffer)
	{
		auto variant_buffer = PackedByteArray();
		// TODO: Investigate better ways to find encoded variant size beforehand
		auto reserve = 64;
		auto variant_size = -1;
		while (true) {
			variant_buffer.resize(reserve);
			variant_size = variant_buffer.encode_var(0, variant, true);
			if (variant_size != -1) {
				// Trim excess from reserve allocations
				variant_buffer.resize(variant_size);
				break;
			}
			reserve *= 2;
		}
		buffer.flint(variant_size);
		buffer.arr(variant_buffer.ptrw(), variant_size);
	}

	Variant read_variant(BufReader& buffer)
	{
		auto variant_size = buffer.flint();
		auto variant_data = buffer.arr(variant_size);
		auto temp_buffer = PackedByteArray();
		temp_buffer.resize(variant_data.size);
		memcpy(temp_buffer.ptrw(), variant_data.data, variant_data.size);

		auto variant = temp_buffer.decode_var(0, true);
		return variant;
	}

	// Usually called with server create_entity()
	void write_entity_data(String node_path, BufWriter& buffer)
	{
		buffer.u8(ObjectType::FILESYSTEM_NODE);
		auto node_path_str = node_path.utf8().get_data();
		buffer.str(node_path_str);
	}

	// Usually called with server register_entity
	void write_entity_data(Node* node, BufWriter& buffer)
	{
		// Write property list + custom properties
		auto entity_variant = (const Variant&) node;
		buffer.u8(ObjectType::INLINE_NODE); // object_type
		auto class_str = node->get_class().utf8().get_data();
		buffer.str(class_str); //node_class

		// https://docs.godotengine.org/en/stable/classes/class_object.html#method-descriptions
		auto infos = node->get_property_list();
		buffer.flint(infos.size()); // property_count
		for (auto i = 0; i < infos.size(); i++) {
			auto info = infos[i];
			auto name = info.get("name"); // Should be string - prop name
			auto type = info.get("type"); // Should be int - variant type
			auto name_str = ((String)name).utf8().get_data();
			buffer.str(name_str);

			// Resources can't be serialised by encode_var, so we reference them by path
			// https://docs.godotengine.org/en/stable/classes/class_packedbytearray.html#class-packedbytearray-method-encode-var
			auto variant = node->get(name);
			Resource* resource;
			Node* node;
			if ((int)type == Variant::OBJECT) {
				resource = Object::cast_to<Resource>(variant);
				node = Object::cast_to<Node>(variant);
			}
			if (resource != nullptr) {
				buffer.u8(ObjectType::FILESYSTEM_RESOURCE); // prop_object_type
				auto path = resource->get_path();
				auto path_str = path.utf8().get_data();
				buffer.str(path_str); // resource_path_str
			}
			else if (node != nullptr) {
				// Handle the unlikely event that the property is a node
				write_entity_data(node, buffer);
			}
			else {
				// Variant
				buffer.u8(ObjectType::VARIANT);
				write_variant(variant, buffer);
			}
		}

		// Write children
		auto child_count = node->get_child_count();
		buffer.flint(child_count);
		for (auto i = 0 ; i < child_count; i++) {
			auto child = node->get_child(i);
			write_entity_data(child, buffer); // node data
		}
	}

	Node* read_entity_data(BufReader& buffer)
	{
		auto object_type = (ObjectType) buffer.u8();
		if (object_type == ObjectType::FILESYSTEM_NODE) {
			// Path to scene where all the info about this node can be loaded is in packet
			auto node_path_str = (string) buffer.str();
			auto node_path = String(node_path_str.c_str());
			Node* entity;
			load_scene(node_path, &entity);
			return entity;
		}
		else if (object_type == ObjectType::INLINE_NODE) {
			// Node, its properties, and its children are defined within the packet
			auto node_class_str = (string) buffer.str();
			auto node_class = String(node_class_str.c_str());

			// Instance node from type name
			auto entity_node = Object::cast_to<Node>(ClassDB::instantiate(node_class));
			if (entity_node == nullptr) {
				UtilityFunctions::print("Couldn't read entity: Node of type ", node_class, " couldn't be instanced with ClassDB.");
				return nullptr;
			}

			// Apply node properties
			auto property_count = buffer.flint();
			for (auto i = 0; i < property_count; i++) {
				auto prop_name_str = (string) buffer.str();
				auto prop_name = String(prop_name_str.c_str());
				auto prop_object_type = buffer.u8();
				if (prop_object_type == ObjectType::INLINE_RESOURCE)  { // i.e: Texture, Material
					// TODO: Implement this! (Likely using compressed_variant)
					UtilityFunctions::print("Couldn't read entity: Inline resource not implemented.");
					return nullptr;
				}
				else if (prop_object_type == ObjectType::FILESYSTEM_RESOURCE)  { // i.e: Mesh
					auto resource_path_str = (string) buffer.str();
					auto resource_path = String(resource_path_str.c_str());
					auto resource_loader = ResourceLoader::get_singleton();

					auto resource = resource_loader->load(resource_path);
					if (!resource.is_valid()) {
						UtilityFunctions::print("Failed to read filesystem resource: Resource was null!");
						continue;
					}
					entity_node->set(prop_name, resource);
				}
				else if (prop_object_type == ObjectType::VARIANT)  { // i.e: Vector3, Color
					// TODO: Optimisation - Look into using hand crafted reading &
					// writing methods for uncompressed variants
					auto variant = read_variant(buffer);
					entity_node->set(prop_name, variant);
				}
			}

			auto child_count = buffer.flint();
			for (auto i =  0; i < child_count; i++) {
				auto child_node = read_entity_data(buffer);
				if (child_node != nullptr) {
					entity_node->add_child(child_node);
				}
			}

			return entity_node;
		}
		else {
			UtilityFunctions::print("Couldn't read entity data, object type was not NODE or INLINE_NODE");
			return nullptr;
		}
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
