#pragma once
#include <godot_cpp/templates/hash_map.hpp>
#include <godot_cpp/variant/vector3.hpp>
#include <godot_cpp/classes/node.hpp>
#include <dataproto_cpp/dataproto.hpp>
#include <map>

using namespace godot;
using namespace dataproto;

namespace NetworkShared {
	enum class ClientPacket : uint8_t {
		// Loading screen / serverlist only
		AUTHENTICATE = 0,

		// Client / player
		SET_CHAT_NAME = 16,
		SET_MODEL_VARIANT = 17,

		UPDATE_MOVEMENT = 32,

		ACTION_GRAB = 48,
		ACTION_DROP = 49,
		ACTION_SWITCH = 50,
		ACTION_USE = 51,
		ACTION_TAKE_DAMAGE = 52,
		ACTION_DIE = 53,
		ACTION_RESPAWN = 54,
		ACTION_CHAT_MESSAGE = 55
	};

	enum class ServerPacket : uint8_t {
		// Loading screen / serverlist only
		SERVER_INFO = 0,

		// Client
		GAME_INFO = 1,
		PLAYERS_INFO = 2,
		ENTITIES_INFO = 3,

		SET_PHASE = 16,

		UPDATE_MOVEMENT = 32,
		UPDATE_HEALTH = 33,
		UPDATE_ENTITY = 36,

		GRAB = 48,
		DROP = 49,
		SWITCH = 50,
		USE = 51,
		TAKE_DAMAGE = 52,
		DIE = 53,
		RESPAWN = 54,
		CHAT_MESSAGE = 55,
		TP_PLAYER = 56
	};

	const std::map<ClientPacket, std::string> _client_packet_names = {
        {ClientPacket::AUTHENTICATE, "AUTHENTICATE"},
        {ClientPacket::SET_CHAT_NAME, "SET_CHAT_NAME"},
        {ClientPacket::SET_MODEL_VARIANT, "SET_MODEL_VARIANT"},
        {ClientPacket::UPDATE_MOVEMENT, "UPDATE_MOVEMENT"},
        {ClientPacket::ACTION_GRAB, "ACTION_GRAB"},
        {ClientPacket::ACTION_DROP, "ACTION_DROP"},
        {ClientPacket::ACTION_SWITCH, "ACTION_SWITCH"},
        {ClientPacket::ACTION_USE, "ACTION_USE"},
        {ClientPacket::ACTION_TAKE_DAMAGE, "ACTION_TAKE_DAMAGE"},
        {ClientPacket::ACTION_DIE, "ACTION_DIE"},
        {ClientPacket::ACTION_RESPAWN, "ACTION_RESPAWN"},
        {ClientPacket::ACTION_CHAT_MESSAGE, "ACTION_CHAT_MESSAGE"}
    };

    const std::map<ServerPacket, std::string> _server_packet_names = {
        {ServerPacket::SERVER_INFO, "SERVER_INFO"},
        {ServerPacket::GAME_INFO, "GAME_INFO"},
        {ServerPacket::PLAYERS_INFO, "PLAYERS_INFO"},
        {ServerPacket::ENTITIES_INFO, "ENTITIES_INFO"},
        {ServerPacket::SET_PHASE, "SET_PHASE"},
        {ServerPacket::UPDATE_MOVEMENT, "UPDATE_MOVEMENT"},
        {ServerPacket::UPDATE_HEALTH, "UPDATE_HEALTH"},
        {ServerPacket::UPDATE_ENTITY, "UPDATE_ENTITY"},
        {ServerPacket::GRAB, "GRAB"},
        {ServerPacket::DROP, "DROP"},
        {ServerPacket::SWITCH, "SWITCH"},
        {ServerPacket::USE, "USE"},
        {ServerPacket::TAKE_DAMAGE, "TAKE_DAMAGE"},
        {ServerPacket::DIE, "DIE"},
        {ServerPacket::RESPAWN, "RESPAWN"},
        {ServerPacket::CHAT_MESSAGE, "CHAT_MESSAGE"},
        {ServerPacket::TP_PLAYER, "TP_PLAYER"}
    };

    template<typename T>
    String to_string(T value)
    {
        if constexpr (std::is_same<T, ClientPacket>::value) {
            auto it = _client_packet_names.find(value);
            if (it != _client_packet_names.end()) {
                return String(it->second.c_str());
            }
        }
        else if constexpr (std::is_same<T, ServerPacket>::value) {
            auto it = _server_packet_names.find(value);
            if (it != _server_packet_names.end()) {
                return String(it->second.c_str());
            }
        }
        return String("UNKNOWN");
    }

	inline uint8_t to_uint8(ClientPacket packet_enum)
	{
    	return static_cast<uint8_t>(packet_enum);
	}

	inline uint8_t to_uint8(ServerPacket packet_enum)
	{
		return static_cast<uint8_t>(packet_enum);
	}

	/**
	 * @brief Serialised properties / can either be defined as a pointer to the filesystem,
	 * or as an inlinne encoded variant. The same case for resource-type properties
	 * on a node.
	 */
	enum ObjectType {
		PACKED_SCENE,
		FILESYSTEM_SCENE,
		INLINE_RESOURCE,
		FILESYSTEM_RESOURCE,
		COMPRESSED_VARIANT,
		VARIANT
	};
	void write_compressed_variant(const Variant& variant, BufWriter& buffer);
	Variant read_compressed_variant(BufReader& buffer);
	/**
	 * @brief Will attempt to write variant using handwritten dataproto handlers, or uncompressed
	 * godot encode_var if no handler is found.
	 * 
	 * @param variant Variant value to be written to buffer
	 * @param buffer Dataproto buffer that variant is to be written to
	 */
	void write_variant(const Variant& variant, BufWriter& buffer);
	/**
	 * @brief Reads a Variant from the provided buffer encoded by write_variant.
	 *
	 * @param buffer The BufReader object from which to read the data.
	 * @return Variant The Variant object constructed from the buffer data.
	 */
	Variant read_variant(BufReader& buffer);
	void write_godot_variant(const Variant& variant, BufWriter& buffer);
	Variant read_godot_variant(BufReader& buffer);
	void write_scene_data(Node* node, BufWriter& buffer);
	void write_scene_data(String node_path, BufWriter& buffer);
	Node* read_scene_data(BufReader& buffer);

    void write_vector2(BufWriter& buffer, Vector2 vector);
    Vector2 read_vector2(BufReader& buffer);
    void write_color(BufWriter& buffer, Color color);
    Color read_color(BufReader& buffer);
	void write_vector3(BufWriter& buffer, Vector3 vector);
	Vector3 read_vector3(BufReader& buffer);
}
