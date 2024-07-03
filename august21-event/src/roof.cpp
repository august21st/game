#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/variant/signal.hpp>
#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/classes/web_socket_peer.hpp>
#include <dataproto_cpp/dataproto.hpp>

#include "roof.hpp"

using namespace godot;

Roof::Roof()
{
}

Roof::~Roof()
{
}

void Roof::_bind_methods()
{
}

void Roof::_ready()
{
	_engine = _engine->get_singleton();
	if (_engine->is_editor_hint()) {
		return;
	}
}

void Roof::_process(double delta)
{
	if (_engine->is_editor_hint()) {
		return;
	}
}