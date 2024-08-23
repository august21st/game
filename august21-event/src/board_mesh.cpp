#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/packed_byte_array.hpp>
#include <godot_cpp/classes/http_request.hpp>
#include <godot_cpp/classes/json.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/classes/image.hpp>
#include <godot_cpp/classes/image_texture.hpp>
#include <godot_cpp/classes/mesh_instance3d.hpp>
#include <godot_cpp/classes/base_material3d.hpp>
#include <godot_cpp/classes/standard_material3d.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

#include "board_mesh.hpp"
#include "godot_cpp/variant/packed_color_array.hpp"

using namespace godot;

BoardMesh::BoardMesh() : _board_width(0), _board_height(0), _board_loaded(false),
	_palette_loaded(false), _generating_texture(false), _board_request(nullptr),
	_metadata_request(nullptr), _board(nullptr), _palette(nullptr)
{
}

BoardMesh::~BoardMesh()
{
	if (_board) {
		delete _board;
		_board = nullptr;
	}
	if (_palette) {
		delete _palette;
		_palette = nullptr;
	}
}

void BoardMesh::_bind_methods()
{
	ClassDB::bind_method(D_METHOD("_on_board_request_completed", "result", "response_code", "headers", "body"),
		&BoardMesh::_on_board_request_completed);
	ClassDB::bind_method(D_METHOD("_on_metadata_request_completed", "result", "response_code", "headers", "body"),
		&BoardMesh::_on_metadata_request_completed);
	ADD_SIGNAL(MethodInfo("board_downloaded", PropertyInfo(Variant::INT, "error"),
		PropertyInfo(Variant::PACKED_BYTE_ARRAY, "board")));
	ADD_SIGNAL(MethodInfo("palette_downloaded", PropertyInfo(Variant::INT, "error"),
		PropertyInfo(Variant::PACKED_COLOR_ARRAY, "palette")));
}

void BoardMesh::_ready()
{
	_board_width = 0;
	_board_height = 0;
	_board_request = memnew(HTTPRequest);
	_board_request->connect("request_completed", Callable(this, "_on_board_request_completed"));
	add_child(_board_request);
	_metadata_request = memnew(HTTPRequest);
	_metadata_request->connect("request_completed", Callable(this, "_on_metadata_request_completed"));
	add_child(_metadata_request);
}

Error BoardMesh::load_canvas(String canvas_url, String palette_url)
{
	if (!_board_request || !_metadata_request) {
		UtilityFunctions::printerr("Could not load canvas: board or metadata request objects are null");
		return Error::FAILED;
	}

	_board_loaded = false;
	_palette_loaded = false;

	_board_request->cancel_request();
	auto board_error = _board_request->request(canvas_url);
	if (board_error != Error::OK) {
		UtilityFunctions::printerr(board_error);
		return board_error;
	}

	_metadata_request->cancel_request();
	auto palette_error = _metadata_request->request(palette_url);
	if (palette_error != Error::OK) {
		UtilityFunctions::printerr(palette_error);
		return palette_error;
	}

	return Error::OK;
}

void BoardMesh::generate_texture()
{
	if (_generating_texture) {
		UtilityFunctions::printerr("Could not generate texture: texture generation already in progress");
		return;
	}
	_generating_texture = true;

	if (get_surface_override_material_count() == 0) {
		UtilityFunctions::printerr("Could not generate texture: meshinstance surface override material count was 0");
		return;
	}

	if (_palette == nullptr || _palette->size() == 0) {
		UtilityFunctions::printerr("Could not generate texture: palette was null or empty");
		return;
	}
	auto palette = *_palette;

	if (_board == nullptr) {
		UtilityFunctions::printerr("Could not generate texture: board was null");
		return;
	}
	auto board = *_board;

	if (!_board_width || !_board_height) {
		UtilityFunctions::printerr("Could not generate texture: board dimensions were invalid");
		return;
	}

	auto image = Image::create(_board_width, _board_height, false, Image::FORMAT_RGBA8);
	for (auto i = 0; i < _board->size(); i++) {
		auto colour_i = board[i];
		auto colour = palette[colour_i < palette.size() ? colour_i : 0];
		image->set_pixel(i % _board_width, i / _board_width, colour);
	}
	auto texture = ImageTexture::create_from_image(image);
	auto surface_material = get_surface_override_material(0);
	if (surface_material.is_valid() && surface_material->is_class("StandardMaterial3D")) {
		const Ref<StandardMaterial3D> standard_material = surface_material;
		standard_material->set_texture(BaseMaterial3D::TEXTURE_ALBEDO, texture);
	}
	else {
		UtilityFunctions::printerr("Could not generate texture: surface override material was not StandardMaterial3D");
	}
	_generating_texture = false;
}

void BoardMesh::_on_board_request_completed(int result, int response_code, const PackedStringArray &headers, const PackedByteArray &body)
{
	if (response_code != HTTPClient::RESPONSE_OK) {
		return;
	}

	_board = memnew(PackedByteArray());
	_board->resize(body.size());
	memcpy(_board->ptrw(), body.ptr(), body.size());
	emit_signal("board_downloaded", _board);

	_board_loaded = true;
	if (_palette_loaded && !_generating_texture) {
		generate_texture();
	}
}

void BoardMesh::_on_metadata_request_completed(int result, int response_code, const PackedStringArray &headers, const PackedByteArray &body)
{
	if (response_code != HTTPClient::RESPONSE_OK) {
		return;
	}

	auto json = JSON::parse_string(body.get_string_from_utf8());
	auto palette_variant = json.get("palette");
	if (palette_variant.get_type() != Variant::Type::ARRAY) {
		UtilityFunctions::printerr("Could not decode metadata: palette param was not an Array");
		return;
	}
	auto palette_array = (Array)palette_variant;
	_palette = memnew(PackedColorArray);

	for (auto i = 0; i < palette_array.size(); i++) {
		Variant colour_variant = palette_array[i];
		if (colour_variant.get_type() != Variant::Type::FLOAT && colour_variant.get_type() != Variant::Type::INT) {
			UtilityFunctions::printerr("Could not decode metadata: palette value was not a number");
			return;
		}

		auto colour_int = (int)colour_variant;
		auto colour = Color(((colour_int >> 24) & 255) / 255.0f, ((colour_int >> 16) & 255) / 255.0f,
			((colour_int >> 8) & 255) / 255.0f, (colour_int & 255) / 255.0f);
		_palette->push_back(colour);
	}

	auto width_variant = json.get("width");
	if (width_variant.get_type() != Variant::Type::FLOAT && width_variant.get_type() != Variant::Type::INT) {
		UtilityFunctions::printerr("Could not decode metadata: width param was not a number");
		return;
	}
	_board_width = (int)width_variant;

	auto height_variant = json.get("height");
	if (height_variant.get_type() != Variant::Type::FLOAT && height_variant.get_type() != Variant::Type::INT) {
		UtilityFunctions::printerr("Could not decode metadata: height param was not a number");
		return;
	}
	_board_height = (int)height_variant;

	_palette_loaded = true;
	if (_board_loaded && !_generating_texture) {
		generate_texture();
	}
}
