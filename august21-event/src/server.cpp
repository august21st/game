#include <godot_cpp/classes/web_socket_multiplayer_peer.hpp>
#include <godot_cpp/classes/web_socket_peer.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <dataproto_cpp/dataproto.hpp>

#include "server.hpp"
#include "godot_cpp/variant/packed_byte_array.hpp"
#include "network_shared.hpp"

using namespace dataproto;
using namespace godot;
using namespace std;
using namespace NetworkShared;

const int SERVER_PORT = 8082;
const int TPS = 20;

Server::Server()
{
}

Server::~Server()
{
	if (_socket_server.is_valid()) {
		_socket_server->disconnect("client_connected", Callable(this, "_on_client_connected"));
		_socket_server->disconnect("client_disconnected", Callable(this, "_on_client_disconnected"));
		_socket_server->disconnect("data_received", Callable(this, "_on_data_received"));
		_socket_server->close();
	}
}

void Server::_bind_methods()
{
	ClassDB::bind_method(D_METHOD("_on_client_connected", "id"), &Server::_on_client_connected);
	ClassDB::bind_method(D_METHOD("_on_client_disconnected", "id"), &Server::_on_client_disconnected);
	ClassDB::bind_method(D_METHOD("_on_data_received", "id", "data"), &Server::_on_data_received);
}

void Server::_ready()
{
	_os = OS::get_singleton();
	_engine = Engine::get_singleton();
	_display_server = DisplayServer::get_singleton();
	bool is_server = _os->has_feature("dedicated_server") || _display_server->get_name() == "headless";
	if (_engine->is_editor_hint() || !is_server) {
		return;
	}
	UtilityFunctions::print("Starting server...");

	_socket_server = Ref<WebSocketMultiplayerPeer>();
	_socket_server.instantiate();
	_socket_server->connect("client_connected", Callable(this, "_on_client_connected"));
	_socket_server->connect("client_disconnected", Callable(this, "_on_client_disconnected"));
	_socket_server->connect("data_received", Callable(this, "_on_data_received"));
	auto socket_error = _socket_server->create_server(8082);
	if (socket_error != Error::OK) {
		UtilityFunctions::printerr("Failed to start server, received error: ", socket_error);
		return;
	}

	_game_time = 0.0L;
	_engine->set_physics_ticks_per_second(TPS);
	set_physics_process(true);
}

void Server::_physics_process(double delta)
{
	// Handle new WS updates
	if (_socket_server.is_valid()) {
		_socket_server->poll();
	}

	// Run game loop
	_game_time += delta;
}

void Server::_on_client_connected(int id)
{
	Ref<WebSocketPeer> client = _socket_server->get_peer(id);
	_clients[id] = client;
	UtilityFunctions::print("Client connected: ", id);
}

void Server::_on_client_disconnected(int id)
{
	_clients.erase(id);
	UtilityFunctions::print("Client disconnected: ", id);
}

void Server::send_to_all(BufWriter packet)
{
	send_to_all(packet.data(), packet.size());
}

void Server::send_to_all(const char* data, size_t size)
{
    PackedByteArray packed_data;
    packed_data.resize(size);
    memcpy(packed_data.ptrw(), data, size);

    for (auto &[id, client] : _clients) {
        client->put_packet(packed_data);
    }
}

void Server::send(int id, BufWriter packet)
{
	send(id, packet.data(), packet.size());
}

void Server::send(int id, const char* data, size_t size)
{
	PackedByteArray packed_data;
    packed_data.resize(size);
    memcpy(packed_data.ptrw(), data, size);
    _clients[id]->put_packet(packed_data);
}

void Server::_on_data_received(int id, const PackedByteArray &data)
{
	UtilityFunctions::print("Data received from client ", id, ": ", data);
	auto packet = BufReader((char*) data.ptr(), data.size());
	auto code = packet.u8();
	switch (code) {
		case ClientPacket::AUTHENTICATE_LINKAGE: {
			auto link_key = packet.str();
			// TODO: Authenticate link key with server, set user int ID, retrieve chat name... etc
			break;
		}
		case ClientPacket::MOVEMENT_UPDATE: {
			break;
		}
		case ClientPacket::DROP_ACTION: {
			break;
		}
		case ClientPacket::GRAB_ACTION: {
			break;
		}
		case ClientPacket::CHAT_MESSAGE: {
			auto message = packet.str();
			auto chat_packet = BufWriter();
			chat_packet.u8(ServerPacket::CLIENT_CHAT_MESSAGE);
			chat_packet.i32(0); // TODO: Entity ID
			chat_packet.i32(0); // TODO: User Int ID
			chat_packet.str(message);
			send_to_all(chat_packet.data(), chat_packet.size());
			break;
		}
	}

	_clients[id]->put_packet(data);
}
