#include <godot_cpp/classes/web_socket_peer.hpp>

#include "client_data.hpp"
#include "entity_info.hpp"

using namespace godot;

ClientData::ClientData(Ref<WebSocketPeer> socket, EntityPlayer* player_entity)
{
	_socket = socket;
	_player_entity = player_entity;
}

ClientData::~ClientData()
{
	if (_socket.is_valid() && _socket->get_ready_state() == WebSocketPeer::STATE_OPEN ) {
		_socket->close(4000, "Connection closed");
	}
	_player_entity->queue_free();
}

Ref<WebSocketPeer> ClientData::get_socket()
{
	return _socket;
}

EntityPlayer* ClientData::get_entity()
{
	return _player_entity;
}

void ClientData::authenticate()
{
	// TODO: Implement this stub
	_authenticated = true;
}

bool ClientData::is_authenticated()
{
	return _authenticated;
}
