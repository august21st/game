#pragma once

namespace NetworkShared {
	enum ServerPacket {
		LINK_RESPONSE = 0,
		GAME_INFO = 1,
		PLAYERS_INFO = 2,
		SET_PHASE = 15,
		KILL_PLAYER = 19,
		UPDATE_PLAYER = 16,
		TP_PLAYER = 21,
		CREATE_ENTITY = 22,
		UPDATE_ENTITY = 23,
		DELETE_ENTITY = 24,
		CHAT_MESSAGE = 31
	};

	enum ClientPacket {
		LINK_KEY = 0,
		UPDATE = 16,
		ACTION_GRAB = 17,
		ACTION_DROP = 18,
		SEND_CHAT_MESSAGE = 31
	};
}
