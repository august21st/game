#include <godot_cpp/classes/http_request.hpp>
#include "board_service.hpp"

using namespace godot;

namespace BoardService {
    vector<uint8_t> get_board()
	{
		auto board = vector<uint8_t>();
		//auto request = Ref<HTTPRequest>();
		//request.instantiate();
		//auto response = request->request("https://raw.githubusercontent.com/rplacetk/canvas1/main/place");		
		
		return board;
	}

    vector<uint32_t> get_palette()
	{
		auto palette = vector<uint32_t>();
		return palette;
	}
}