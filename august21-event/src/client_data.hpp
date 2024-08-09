#include <godot_cpp/classes/web_socket_peer.hpp>

#include "entity_player.hpp"

using namespace godot;

class ClientData {
private:
	Ref<WebSocketPeer> _socket;
	EntityPlayer* _player_entity;

public:
	ClientData(Ref<WebSocketPeer> socket, EntityPlayer* player_entity);
	~ClientData();
	Ref<WebSocketPeer> get_socket();
	EntityPlayer* get_entity();
};
