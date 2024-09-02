#pragma once

namespace NetworkShared {
	enum ServerPacket {
		// Loading screen / serverlist only
		SERVER_INFO = 0,
		// Client
		GAME_INFO = 1,
		PLAYERS_INFO = 2,
		SET_PHASE = 3,
		UPDATE_PLAYER_HEALTH = 15,
		UPDATE_PLAYER_MOVEMENT = 16,
		TP_PLAYER = 21,
		ENTITIES_INFO = 22,
		UPDATE_ENTITY = 23,
		CHAT_MESSAGE = 31
	};

	enum ClientPacket {
		AUTHENTICATE = 0,
		SET_CHAT_NAME = 1,
		SET_MODEL_VARIANT = 2,
		UPDATE_MOVEMENT = 16,
		ACTION_GRAB = 17,
		ACTION_DROP = 18,
		ACTION_USE = 19,
		ACTION_TAKE_DAMAGE = 20,
		ACTION_CHAT_MESSAGE = 31
	};

	bool is_server();
}
