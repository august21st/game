#pragma once

namespace NetworkShared {
	enum ServerPacket {
		CONFIGURATION = 0,
		GAME_INFO = 1,
		START = 2,
		ENTITY_CREATE = 16,
		ENTITY_UPDATE = 17,
		ENTITY_DELETE = 18,
		CLIENT_CHAT_MESSAGE = 20
	};

	enum ClientPacket {
		AUTHENTICATE_LINKAGE = 15,
		MOVEMENT_UPDATE = 16,
		GRAB_ACTION = 17,
		DROP_ACTION = 18,
		CHAT_MESSAGE = 20
	};
}
