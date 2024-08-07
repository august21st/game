#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/variant/signal.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/variant/callable.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/method_bind.hpp>
#include <dataproto_cpp/dataproto.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/classes/window.hpp>
#include <godot_cpp/classes/directional_light3d.hpp>

#include "client.hpp"
#include "entity_player.hpp"

using namespace godot;
using namespace dataproto;

EntityPlayer::EntityPlayer()
{
}

EntityPlayer::~EntityPlayer()
{
}

void EntityPlayer::_bind_methods()
{
}

void EntityPlayer::_ready()
{
	_engine = Engine::get_singleton();
	if (_engine->is_editor_hint()) {
		return;
	}
}

void EntityPlayer::set_chat_name(String name)
{
	_chat_name = name;
}

String EntityPlayer::get_chat_name()
{
	return _chat_name;
}
