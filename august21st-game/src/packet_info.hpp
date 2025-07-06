#pragma once
#include <godot_cpp/classes/object.hpp>
#include <godot_cpp/core/class_db.hpp>

using namespace godot;

// TODO: THis should be an object - something is wrong here..
class PacketInfo : public Object {
	GDCLASS(PacketInfo, Object)

protected:
	static void _bind_methods();

public:
	int code = -1;
	String code_name = "";
	String from = "";
	String to = "";
	String time = "";
	int item_index = -1;

	PacketInfo();
	~PacketInfo();
};
