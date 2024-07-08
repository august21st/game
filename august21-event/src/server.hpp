#pragma once
#include <godot_cpp/classes/os.hpp>
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/display_server.hpp>
#include <map>

using namespace std;
using namespace godot;

class Server : public Node {
	GDCLASS(Server, Node);

protected:
	OS* _os;
	Engine* _engine;
	DisplayServer* _display_server;
	map<int, Node*> _entities;

public:
	Server();
	~Server();
	void _ready() override;
	//void _process(double delta) override;
};