#pragma once
#include <cstdint>
#include <vector>

using namespace std;

namespace BoardService {
    vector<uint8_t> get_board();
    vector<uint32_t> get_palette();
}