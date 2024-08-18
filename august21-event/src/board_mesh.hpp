#pragma once
#include <godot_cpp/variant/packed_color_array.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/packed_byte_array.hpp>
#include <godot_cpp/classes/http_request.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/classes/mesh_instance3d.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

using namespace godot;

#define DEFAULT_CANVAS_URL "https://raw.githubusercontent.com/rplacetk/canvas1/main/place"
#define DEFAULT_METADATA_URL "https://raw.githubusercontent.com/rplacetk/canvas1/main/metadata.json"

class BoardMesh : public MeshInstance3D {
    GDCLASS(BoardMesh, MeshInstance3D)

protected:
    Engine* _engine;
    static void _bind_methods();
    int _board_width;
    int _board_height;
    bool _board_loaded;
    bool _palette_loaded;
    bool _generating_texture;
	HTTPRequest* _board_request;
    HTTPRequest* _metadata_request;
    PackedByteArray* _board;
    PackedColorArray* _palette;
    void generate_texture();

public:
    BoardMesh();
    ~BoardMesh();
    void _ready() override;
    Error load_canvas(String canvas_url = DEFAULT_CANVAS_URL, String metadata_url = DEFAULT_METADATA_URL);
    void _on_board_request_completed(int result, int response_code, const PackedStringArray &headers, const PackedByteArray &body);
    void _on_metadata_request_completed(int result, int response_code, const PackedStringArray &headers, const PackedByteArray &body);
};
