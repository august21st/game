#include "game_root.hpp"

GameRoot::GameRoot()
{
}

GameRoot::~GameRoot()
{
}

void GameRoot::_bind_methods()
{
}

String GameRoot::get_current_phase_scene()
{
	return "";
}

String GameRoot::get_current_phase_event()
{
	return "";
}

bool GameRoot::has_phase_scene(String name)
{
	return false;
}

Node* GameRoot::get_phase_scene(String name)
{
	return nullptr;
}

bool GameRoot::is_server()
{
	return false;
}

bool GameRoot::is_client()
{
	return false;
}
