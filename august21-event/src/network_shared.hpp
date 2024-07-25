#pragma once

namespace NetworkShared {
	enum ServerPacket {
		LINK_RESPONSE = 0,
		GAME_INFO = 1,
		SET_PHASE = 15,
		CREATE_ENTITY = 16,
		UPDATE_ENTITY = 17,
		DELETE_ENTITY = 18,
		KILL_PLAYER = 19,
		TP_PLAYER = 21,
		CHAT_MESSAGE = 31
	};

	enum ClientPacket {
		LINK_KEY = 0,
		UPDATE_POSITION = 16,
		ACTION_GRAB = 17,
		ACTION_DROP = 18,
		SEND_CHAT_MESSAGE = 31
	};
}
