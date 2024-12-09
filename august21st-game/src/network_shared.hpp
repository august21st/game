#pragma once
#include <godot_cpp/variant/vector3.hpp>
#include <dataproto_cpp/dataproto.hpp>

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

		UPDATE_PLAYER_MOVEMENT = 32,
		UPDATE_PLAYER_HEALTH = 33,
		UPDATE_ENTITY = 36,

		GRAB = 48,
		DROP = 49,
		SWITCH = 50,
		USE = 51,
		DAMAGE = 52,
		DIE = 53,
		RESPAWN = 54,
		CHAT_MESSAGE = 55,
		TP_PLAYER = 56
	};

	inline uint8_t to_uint8(ClientPacket packet_enum) {
    	return static_cast<uint8_t>(packet_enum);
	}

	inline uint8_t to_uint8(ServerPacket packet_enum) {
		return static_cast<uint8_t>(packet_enum);
	}

	void write_vector3(BufWriter& packet, Vector3 vector);
	Vector3 read_vector3(BufReader& packet);
}
