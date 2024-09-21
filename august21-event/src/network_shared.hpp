#pragma once
#include <godot_cpp/variant/vector3.hpp>
#include <dataproto_cpp/dataproto.hpp>

using namespace godot;
using namespace dataproto;

namespace NetworkShared {
	enum ClientPacket {
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
		ACTION_CHAT_MESSAGE = 53
	};

	enum ServerPacket {
		// Loading screen / serverlist only
		SERVER_INFO = 0,

		// Client
		GAME_INFO = 1,
		PLAYERS_INFO = 2,
		ENTITIES_INFO = 3,

		SET_PHASE = 16,

		UPDATE_PLAYER_HEALTH = 32,
		UPDATE_PLAYER_MOVEMENT = 33,

		TP_PLAYER = 34,
		UPDATE_ENTITY = 36,
		GRAB = 48,
		DROP = 49,
		SWITCH = 50,
		USE = 51,
		CHAT_MESSAGE = 53
	};

	bool is_server();
	void write_vector3(BufWriter& packet, Vector3 vector);
	Vector3 read_vector3(BufReader& packet);
}
