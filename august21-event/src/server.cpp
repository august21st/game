#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/display_server.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/os.hpp>
#include <map>

#include "server.hpp"

using namespace godot;

Server::Server()
{
}

Server::~Server()
{
}

void Server::_bind_methods()
{
}

void Server::_ready()
{
    _os = OS::get_singleton();
    _engine = Engine::get_singleton();
    _display_server = DisplayServer::get_singleton();
    auto is_server = _os->has_feature("dedicated_server") || _display_server->get_name() == "headless";
    if (_engine->is_editor_hint() || !is_server) {
        return;
    }

    _entities = { };
}