#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/classes/packed_scene.hpp>
#include <godot_cpp/classes/file_access.hpp>
#include <dataproto_cpp/dataproto.hpp>

#include "network_shared.hpp"
#include "godot_cpp/variant/packed_byte_array.hpp"
#include "godot_cpp/variant/string.hpp"
#include "godot_cpp/variant/utility_functions.hpp"
#include "godot_cpp/variant/vector3.hpp"
#include "node_shared.hpp"

using namespace dataproto;
using namespace godot;
using namespace NodeShared;

namespace NetworkShared {
	void write_compressed_variant(const Variant& variant, BufWriter& buffer)
	{
		auto variant_buffer = PackedByteArray();
		auto reserve = 64;
		int variant_size = -1;
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

		UtilityFunctions::print("DEBUG: Write compressed variant: variant size:", variant_size,
			" buffer size:", variant_buffer.size());
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

	void write_godot_variant(const Variant& variant, BufWriter& buffer)
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

	void write_variant(const Variant& variant, BufWriter& buffer)
	{
		switch (variant.get_type()) {
			case Variant::Type::NIL: {
				buffer.u8(Variant::Type::NIL);
				break;
			}
			case Variant::Type::BOOL: {
				buffer.u8(Variant::Type::BOOL);
				buffer.u8(variant.operator bool());
				break;
			}
			case Variant::Type::INT: {
				buffer.u8(Variant::Type::INT);
				buffer.i64(variant.operator int64_t());
				break;
			}
			case Variant::Type::FLOAT: {
				buffer.u8(Variant::Type::FLOAT);
				buffer.f64(variant.operator double());
				break;
			}
			case Variant::Type::STRING: {
				buffer.u8(Variant::Type::STRING);
				buffer.str(variant.operator String().utf8().get_data());
				break;
			}
			case Variant::Type::STRING_NAME: {
				buffer.u8(Variant::Type::STRING_NAME);
				auto string_name_variant = variant.operator StringName();
				auto str_buffer = string_name_variant.to_utf8_buffer();
				buffer.str(str_buffer.ptrw(), str_buffer.size()); // TODO Consider using .arr
				break;
			}
			case Variant::Type::VECTOR2: {
				buffer.u8(Variant::Type::VECTOR2);
				write_vector2(buffer, variant.operator Vector2());
				break;
			}
			case Variant::Type::VECTOR3: {
				buffer.u8(Variant::Type::VECTOR3);
				write_vector3(buffer, variant.operator Vector3());
				break;
			}
			case Variant::Type::COLOR: {
				buffer.u8(Variant::Type::COLOR);
				write_color(buffer, variant.operator Color());
				break;
			}
			case Variant::Type::DICTIONARY: {
				buffer.u8(Variant::Type::DICTIONARY);
				{
					Dictionary dict = variant.operator Dictionary();
					buffer.flint(dict.size());
					Array keys = dict.keys();
					Array values = dict.values();
					for (int i = 0; i < dict.size(); i++) {
						write_variant(keys[i], buffer);
						write_variant(values[i], buffer);
					}
				}
				break;
			}
			case Variant::Type::ARRAY: {
				buffer.u8(Variant::Type::ARRAY);
				{
					Array array = variant.operator Array();
					buffer.flint(array.size());
					for (int i = 0; i < array.size(); i++) {
						write_variant(array[i], buffer);
					}
				}
				break;
			}
			default: {
				buffer.u8(variant.get_type());
				write_godot_variant(variant, buffer);
				break;
			}
		}
	}

	Variant read_godot_variant(BufReader& buffer)
	{
		auto variant_size = buffer.flint();
		auto variant_data = buffer.arr(variant_size);
		auto temp_buffer = PackedByteArray();
		temp_buffer.resize(variant_data.size);
		memcpy(temp_buffer.ptrw(), variant_data.data, variant_data.size);

		auto variant = temp_buffer.decode_var(0, true);
		return variant;
	}

	Variant read_variant(BufReader& buffer)
	{
		auto type = (Variant::Type) buffer.u8();
		switch (type) {
			case Variant::Type::NIL: {
				return Variant();
			}
			case Variant::Type::BOOL: {
				return Variant((bool) buffer.u8());
			}
			case Variant::Type::INT: {
				return Variant((int64_t) buffer.i64());
			}
			case Variant::Type::FLOAT: {
				return Variant((double) buffer.f64());
			}
			case Variant::Type::STRING: {
				auto variant_str = (string) buffer.str();
				auto string = String(variant_str.c_str());
				return string;
			}
			case Variant::Type::STRING_NAME: {
				auto str_buffer = (string) buffer.str();
				auto string = String(str_buffer.c_str());
				auto string_name = StringName(string);
				return string_name;
			}
			case Variant::Type::VECTOR2: {
				return read_vector2(buffer);
			}
			case Variant::Type::VECTOR3: {
				return read_vector3(buffer);
			}
			case Variant::Type::COLOR: {
				return read_color(buffer);
			}
			case Variant::Type::DICTIONARY: {
				Dictionary dict;
				uint32_t size = buffer.u32();
				for (uint32_t i = 0; i < size; i++) {
					Variant key = read_variant(buffer);
					Variant value = read_variant(buffer);
					dict[key] = value;
				}
				return Variant(dict);
			}
			case Variant::Type::ARRAY: {
				Array array;
				uint32_t size = buffer.u32();
				for (uint32_t i = 0; i < size; i++) {
					array.push_back(read_variant(buffer));
				}
				return Variant(array);
			}
			default: {
				return read_godot_variant(buffer);
			}
		}
	}

	// Usually called with server create_entity()
	void write_scene_data(String node_path, BufWriter& buffer)
	{
		buffer.u8(ObjectType::FILESYSTEM_SCENE);
		auto node_path_str = node_path.utf8().get_data();
		buffer.str(node_path_str);
	}

	void write_scene_data(Node* node, BufWriter& buffer)
	{
		buffer.u8(ObjectType::PACKED_SCENE);
		Ref<PackedScene> packed_scene = memnew(PackedScene);
		packed_scene->pack(node);
		Dictionary bundled = packed_scene->get("_bundled");

		UtilityFunctions::print("DEBUG: Write entity data: ", bundled);
		write_variant(bundled, buffer);
		UtilityFunctions::print("DEBUG: Write entity data: afterwards buffer length:", buffer.size());
		String debug_str = "DEBUG: Write entity data: raw data:\n";
		for (auto i = 0; i < buffer.size(); i++) {
			debug_str += String("{0} ").format(Array::make((uint8_t)buffer.data()[i]));
		}
		UtilityFunctions::print(debug_str);
	}

	Node* read_scene_data(BufReader& buffer)
	{
		auto object_type = (ObjectType) buffer.u8();
		if (object_type == ObjectType::FILESYSTEM_SCENE) {
			// Path to scene where all the info about this node can be loaded is in packet
			auto node_path_str = (string) buffer.str();
			auto node_path = String(node_path_str.c_str());
			Node* node = nullptr;
			load_scene(node_path, &node);
			return node;
		}
		else if (object_type == ObjectType::PACKED_SCENE) {
			Dictionary bundled = read_compressed_variant(buffer);
			Ref<PackedScene> packed_scene = memnew(PackedScene);
			// https://docs.godotengine.org/en/stable/classes/class_packedscene.html#class-packedscene-property-bundled
			packed_scene->set("_bundled", bundled);
			Node* scene_instance = packed_scene->instantiate();
			return scene_instance;
		}
		return nullptr;
	}

	void write_vector2(BufWriter& buffer, Vector2 vector)
	{
		buffer.f32(vector.x);
		buffer.f32(vector.y);
	}

	Vector2 read_vector2(BufReader& buffer)
	{
		auto x = buffer.f32();
		auto y = buffer.f32();
		return Vector2(x, y);
	}

	void write_color(BufWriter& buffer, Color color)
	{
		buffer.f32(color.r);
		buffer.f32(color.g);
		buffer.f32(color.b);
		buffer.f32(color.a);
	}

	Color read_color(BufReader& buffer)
	{
		auto r = buffer.f32();
		auto g = buffer.f32();
		auto b = buffer.f32();
		auto a = buffer.f32();
		return Color(r, g, b, a);
	}

	void write_vector3(BufWriter& buffer, Vector3 vector)
	{
		buffer.f32(vector.x);
		buffer.f32(vector.y);
		buffer.f32(vector.z);
	}

	Vector3 read_vector3(BufReader& buffer)
	{
		auto x = buffer.f32();
		auto y = buffer.f32();
		auto z = buffer.f32();
		return Vector3(x, y, z);
	}
}
