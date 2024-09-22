/**
 * The contents of this file are based off the original work [godot-simple-portal-system]
 * (https://github.com/Donitzo/godot-simple-portal-system/blob/main/src/scripts/portal.gd),
 * by [Donitzo](https://github.com/Donitzo), released freely into the public domain under the
 * [Creative Commons Zero v1.0 Universal license](https://creativecommons.org/publicdomain/zero/1.0/deed.en).
 *
 * ### Attribution:
 * - Original source by [Donitzo](https://github.com/Donitzo).
 * - Source port to C++ & adaptations by Zekiah-A & contributors.
 *
 * ### Disclaimer:
 * This adaptation is not endorsed by the original licensor and is provided without warranties,
 * this adaptation is also public domain, and is not licensed under the project's primary license, see
 * ([Creative Commons Zero v1.0 Universal license](https://creativecommons.org/publicdomain/zero/1.0/deed.en)).
 */
#include <godot_cpp/classes/camera3d.hpp>
#include <godot_cpp/classes/environment.hpp>
#include <godot_cpp/classes/global_constants.hpp>
#include <godot_cpp/classes/material.hpp>
#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/classes/shader_material.hpp>
#include <godot_cpp/classes/sub_viewport.hpp>
#include <godot_cpp/classes/viewport.hpp>
#include <godot_cpp/classes/viewport_texture.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/math.hpp>
#include <godot_cpp/variant/aabb.hpp>
#include <godot_cpp/variant/basis.hpp>
#include <godot_cpp/variant/node_path.hpp>
#include <godot_cpp/variant/transform3d.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/variant/vector3.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/classes/window.hpp>
#include <godot_cpp/classes/area3d.hpp>

#include "portal.hpp"
#include "client.hpp"
#include "node_shared.hpp"

using namespace NodeShared;

Portal::Portal() : _resource_loader(nullptr), _client(nullptr),
	_main_camera(nullptr), _exit_environment_override(nullptr), _exit_portal(nullptr),
	_viewport(nullptr), _exit_camera(nullptr), _material_override(nullptr), _entrance_area(nullptr),
	_main_camera_path(NodePath()), _exit_environment_override_path(NodePath()), _exit_portal_path(NodePath()),
	_entrance_area_path(NodePath())
{
}

Portal::~Portal()
{
}

void Portal::_bind_methods()
{
	ClassDB::bind_method(D_METHOD("get_viewport_vertical_resolution"), &Portal::get_viewport_vertical_resolution);
	ClassDB::bind_method(D_METHOD("set_viewport_vertical_resolution"), &Portal::set_viewport_vertical_resolution);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "viewport_vertical_resolution"),
			"set_viewport_vertical_resolution", "get_viewport_vertical_resolution");
	ClassDB::bind_method(D_METHOD("get_disable_viewport_distance"), &Portal::get_disable_viewport_distance);
	ClassDB::bind_method(D_METHOD("set_disable_viewport_distance"), &Portal::set_disable_viewport_distance);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "viewport_disable_distance"),
			"set_disable_viewport_distance", "get_disable_viewport_distance");
	ClassDB::bind_method(D_METHOD("get_destroy_disabled_viewport"), &Portal::get_destroy_disabled_viewport);
	ClassDB::bind_method(D_METHOD("set_destroy_disabled_viewport"), &Portal::set_destroy_disabled_viewport);
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "destroy_disabled_viewport"),
			"set_destroy_disabled_viewport", "get_destroy_disabled_viewport");
	ClassDB::bind_method(D_METHOD("get_fade_out_distance_max"), &Portal::get_fade_out_distance_max);
	ClassDB::bind_method(D_METHOD("set_fade_out_distance_max"), &Portal::set_fade_out_distance_max);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "fade_out_distance_max"),
			"set_fade_out_distance_max", "get_fade_out_distance_max");
	ClassDB::bind_method(D_METHOD("get_fade_out_distance_min"), &Portal::get_fade_out_distance_min);
	ClassDB::bind_method(D_METHOD("set_fade_out_distance_min"), &Portal::set_fade_out_distance_min);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "fade_out_distance_min"),
			"set_fade_out_distance_min", "get_fade_out_distance_min");
	ClassDB::bind_method(D_METHOD("get_fade_out_colour"), &Portal::get_fade_out_colour);
	ClassDB::bind_method(D_METHOD("set_fade_out_colour"), &Portal::set_fade_out_colour);
	ADD_PROPERTY(PropertyInfo(Variant::COLOR, "fade_out_colour"),
			"set_fade_out_colour", "get_fade_out_colour");
	ClassDB::bind_method(D_METHOD("get_exit_portal_scale"), &Portal::get_exit_portal_scale);
	ClassDB::bind_method(D_METHOD("set_exit_portal_scale"), &Portal::set_exit_portal_scale);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "exit_portal_scale"),
			"set_exit_portal_scale", "get_exit_portal_scale");
	ClassDB::bind_method(D_METHOD("get_exit_near_subtract"), &Portal::get_exit_near_subtract);
	ClassDB::bind_method(D_METHOD("set_exit_near_subtract"), &Portal::set_exit_near_subtract);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "exit_near_subtract"),
			"set_exit_near_subtract", "get_exit_near_subtract");
	ClassDB::bind_method(D_METHOD("get_main_camera_override"), &Portal::get_main_camera_override);
	ClassDB::bind_method(D_METHOD("set_main_camera_override"), &Portal::set_main_camera_override);
	ADD_PROPERTY(PropertyInfo(Variant::NODE_PATH, "main_camera_override", PROPERTY_HINT_NODE_PATH_VALID_TYPES, "Camera3D"),
			"set_main_camera_override", "get_main_camera_override");
	ClassDB::bind_method(D_METHOD("get_exit_environment_override"), &Portal::get_exit_environment_override);
	ClassDB::bind_method(D_METHOD("set_exit_environment_override"), &Portal::set_exit_environment_override);
	ADD_PROPERTY(PropertyInfo(Variant::NODE_PATH, "exit_environment_override", PROPERTY_HINT_NODE_PATH_VALID_TYPES, "Environment"),
			"set_exit_environment_override", "get_exit_environment_override");
	ClassDB::bind_method(D_METHOD("get_exit_portal"), &Portal::get_exit_portal);
	ClassDB::bind_method(D_METHOD("set_exit_portal"), &Portal::set_exit_portal);
	ADD_PROPERTY(PropertyInfo(Variant::NODE_PATH, "exit_portal", PROPERTY_HINT_NODE_PATH_VALID_TYPES, "Portal"),
			"set_exit_portal", "get_exit_portal");
	ClassDB::bind_method(D_METHOD("get_entrance_area"), &Portal::get_entrance_area);
	ClassDB::bind_method(D_METHOD("set_entrance_area"), &Portal::set_entrance_area);
	ADD_PROPERTY(PropertyInfo(Variant::NODE_PATH, "entrance_area", PROPERTY_HINT_NODE_PATH_VALID_TYPES, "Area3D"),
			"set_entrance_area", "get_entrance_area");

	ClassDB::bind_method(D_METHOD("_on_viewport_size_changed"), &Portal::_on_viewport_size_changed);
	ClassDB::bind_method(D_METHOD("_on_current_scene_ready"), &Portal::_on_current_scene_ready);
	ClassDB::bind_method(D_METHOD("_on_entrance_area_body_entered"), &Portal::_on_entrance_area_body_entered);
}

void Portal::_ready()
{
	_client = get_global_client(this);
	if (_client == nullptr) {
		UtilityFunctions::printerr("Could not get client: autoload singleton was null");
		set_process(false);
		return;
	}

	_resource_loader = ResourceLoader::get_singleton();

	_mesh_aabb = get_aabb();

	auto material_override = get_material_override();
	if (material_override == nullptr || !material_override->is_class("ShaderMaterial")) {
		UtilityFunctions::printerr("Portal '", get_name(), "' expects a Geometry 'Material Override' shader material of type portal.gdshader");
		return;
	}
	_material_override = material_override;
	_material_override->set_shader_parameter("fade_out_distance_max", _fade_out_distance_max);
	_material_override->set_shader_parameter("fade_out_distance_min", _fade_out_distance_min);
	_material_override->set_shader_parameter("fade_out_colour", _fade_out_colour);

	auto parent_node3d = Object::cast_to<Node3D>(get_parent());
	if (parent_node3d != nullptr) {
		auto parent_scale = parent_node3d->get_global_transform().get_basis().get_scale();
		if (Math::abs(parent_scale.x - parent_scale.y) > 0.01 || Math::abs(parent_scale.x - parent_scale.z)) {
			UtilityFunctions::push_warning("Portal '", get_name(), "' will not work correctly: parent node is not uniformly scaled");
		}
	}

	// Check again if the setter was called too early (before _ready)
	if (_exit_environment_override == nullptr && !_exit_environment_override_path.is_empty()) {
		// If it's not null now, it will be applied at the point of the camera's creation in create_viewport
		_exit_environment_override = get_node<Environment>(_exit_environment_override_path);
	}

	get_viewport()->connect("size_changed", Callable(this, "_on_viewport_size_changed"));
	// TODO: Make shared functionality of client / server classes, or better yet avoid this hack alltogether
	// by waiting in process until we have actually located our counterpart - would also more easily allow
	// reconnection in case the exit portal is lost or re-assigneed
	_client->connect("current_scene_ready", Callable(this, "_on_current_scene_ready"));

	// Portal must render last such that all cameras have already finished transforming
	set_process_priority(1000);
	// Disable rendering until _on_current_scene_ready
	set_enabled(false);
}

void Portal::_on_current_scene_ready()
{
	// Check again if the setter was called too early (before _ready)
	if (_exit_portal == nullptr && !_exit_portal_path.is_empty()) {
		_exit_portal = get_node<Portal>(_exit_portal_path);
	}

	// Verify that exit portal is now valid and in our tree
	if (_exit_portal == nullptr || !_exit_portal->is_inside_tree() || _exit_portal->get_tree() != get_tree()) {
		UtilityFunctions::printerr("Disabling portal '", get_name(), "': exit portal not found or not within the same SceneTree.");
		set_enabled(false);
	}
	else {
		set_enabled(true);
	}

	// Check again if the setter was called too early (before _ready)
	if (_entrance_area == nullptr && !_entrance_area_path.is_empty()) {
		_entrance_area = get_node<Area3D>(_entrance_area_path);
		if (_entrance_area != nullptr) {
			_entrance_area->connect("body_entered", Callable(this, "_on_entrance_area_body_entered"));
		}
	}

	if (_main_camera == nullptr && !_main_camera_path.is_empty()) {
		// First check override
		_main_camera = get_node<Camera3D>(_main_camera_path);
		if (_main_camera == nullptr) {
			// Next try scene default
			_main_camera = get_viewport()->get_camera_3d();
			if (_main_camera == nullptr) {
				UtilityFunctions::print("Portal '", get_name(), "' cannot render: Scene main camera not found.");
			}
		}
	}

	if (!_destroy_disabled_viewport) {
		create_viewport();
	}
}

void Portal::set_enabled(bool enable)
{
	set_visible(enable);
	set_process(enable);
}

int Portal::get_viewport_vertical_resolution()
{
	return _viewport_vertical_resolution;
}

void Portal::set_viewport_vertical_resolution(int value)
{
	_viewport_vertical_resolution = value;
}

float Portal::get_disable_viewport_distance()
{
	return _disable_viewport_distance;
}

void Portal::set_disable_viewport_distance(float value)
{
	_disable_viewport_distance = value;
}

bool Portal::get_destroy_disabled_viewport()
{
	return _destroy_disabled_viewport;
}

void Portal::set_destroy_disabled_viewport(bool value)
{
	_destroy_disabled_viewport = value;
}

float Portal::get_fade_out_distance_max()
{
	return _fade_out_distance_max;
}

void Portal::set_fade_out_distance_max(float value)
{
	_fade_out_distance_max = value;
}

float Portal::get_fade_out_distance_min()
{
	return _fade_out_distance_min;
}

void Portal::set_fade_out_distance_min(float value)
{
	_fade_out_distance_min = value;
}

Color Portal::get_fade_out_colour()
{
	return _fade_out_colour;
}

void Portal::set_fade_out_colour(Color value)
{
	_fade_out_colour = value;
}

float Portal::get_exit_portal_scale()
{
	return _exit_portal_scale;
}

void Portal::set_exit_portal_scale(float value)
{
	_exit_portal_scale = value;
}

float Portal::get_exit_near_subtract()
{
	return _exit_near_subtract;
}

void Portal::set_exit_near_subtract(float value)
{
	_exit_near_subtract = value;
}

NodePath Portal::get_main_camera_override()
{
	return _main_camera_path;
}

void Portal::set_main_camera_override(const NodePath& path)
{
	_main_camera_path = path;

	if (ready_and_connected() && !path.is_empty()) {
		_main_camera = get_node<Camera3D>(path);
	}
}

NodePath Portal::get_exit_environment_override()
{
	return _exit_environment_override_path;
}

void Portal::set_exit_environment_override(const NodePath& path)
{
	_exit_environment_override_path = path;

	if (ready_and_connected() && !path.is_empty()) {
		_exit_environment_override = get_node<Environment>(path);

		// Exit camera may not exist if unset or called before _ready,
		// _exit_environment_override_path will be checked again in _ready
		if (_exit_camera != nullptr) {
			_exit_camera->set_environment(_exit_environment_override);
		}
	}
}

NodePath Portal::get_exit_portal()
{
	return _exit_portal_path;
}

void Portal::set_exit_portal(const NodePath& path)
{
	_exit_portal_path = path;

	// It may not exist if this was called before ready, or the exit portal isn't
	// ready yet, path will be checked again in _on_current_scene_ready
	if (ready_and_connected() && !path.is_empty()) {
		_exit_portal = get_node<Portal>(path);
		set_enabled(_exit_portal != nullptr);
	}
}

NodePath Portal::get_entrance_area()
{
	return _entrance_area_path;
}

void Portal::set_entrance_area(const NodePath& path)
{
	_entrance_area_path = path;

	// It may not exist if this was called before ready, or the entrance area
	// isn't ready yet, path will be checked again in _on_current_scene_ready
	if (ready_and_connected() && !path.is_empty()) {
		_entrance_area = get_node<Area3D>(path);
		if (_entrance_area != nullptr) {
			_entrance_area->connect("body_entered", Callable(this, "_on_entrance_area_body_entered"));
		}
	}
}

void Portal::_on_entrance_area_body_entered(Node3D* body)
{
	// If we have nowhere to TP to, or it is probably world object, ignore
	if (_exit_portal == nullptr || body->is_class("StaticBody3D")) {
		return;
	}

	auto body_transform = body->get_global_transform();
	auto tp_transform = real_to_exit_transform(body_transform);
	body->set_global_transform(tp_transform);
}

bool Portal::ready_and_connected()
{
	return is_node_ready() && is_inside_tree();
}

void Portal::_on_viewport_size_changed()
{
	// Rate limit resizes to conserve performance
	_seconds_until_resize = RESIZE_THROTTLE_SECONDS;
}

void Portal::create_viewport()
{
	_viewport = memnew(SubViewport());
	_viewport->set_name("Viewport");
	_viewport->set_clear_mode(SubViewport::CLEAR_MODE_ONCE);
	add_child(_viewport);
	_material_override->set_shader_parameter("albedo", _viewport->get_texture());

	// Exit camera which renders portal surface
	_exit_camera = memnew(Camera3D());
	_exit_camera->set_name("Camera");
	if (_exit_environment_override != nullptr) {
		_exit_camera->set_environment(_exit_environment_override);
	}
	_viewport->add_child(_exit_camera);
	// Resize the viewport on the next _process
	_seconds_until_resize = 0;
}

void Portal::_process(double delta)
{
	// Try catch changes to the main camera if it is still null / was unset
	if (_main_camera == nullptr) {
		_main_camera = get_viewport()->get_camera_3d();
	}

	// Disable the viewport if the portal is further away than disable_viewport_distance
	// or if the portal is invisible in the scene tree
	// TODO: This might be better handled using an areabody
	auto global_position = get_global_position();
	auto should_disable_viewport = !is_visible_in_tree() ||
			_main_camera->get_global_position().distance_squared_to(global_position) > Math::pow(_disable_viewport_distance, 2);
	if (should_disable_viewport) {
		if (_viewport != nullptr) {
			_viewport->set_disable_3d(should_disable_viewport);
			// Destroy the disabled viewport to save memory
			_material_override->set_shader_parameter("albedo", nullptr);
			_viewport->queue_free();
			_viewport = nullptr;
		}

		// Ensure the portal can re-size the second it is enabled again
		if (Math::is_nan(_seconds_until_resize)) {
			_seconds_until_resize = 0;
		}
		return;
	}

	// Create viewport
	if (_viewport == nullptr) {
		create_viewport();
	}

	// Throttle the viewport resizing for better performance
	if (!Math::is_nan(_seconds_until_resize)) {
		_seconds_until_resize -= delta;
		if (_seconds_until_resize <= 0) {
			_seconds_until_resize = Math_NAN;

			auto viewport_size = get_viewport()->get_visible_rect().size;
			if (_viewport_vertical_resolution == -1) {
				// Resize the viewport to the main viewport size
				_viewport->set_size(viewport_size);
			} else {
				// Resize the viewport to the fixed height vertical_viewport_resolution and dynamic width
				auto aspect_ratio = float(viewport_size.x) / viewport_size.y;
				_viewport->set_size(Vector2i(int(_viewport_vertical_resolution * aspect_ratio + 0.5), _viewport_vertical_resolution));
			}
		}
	}

	// Move the exit camera relative to the exit portal based on the main camera's position relative to
	// the entrance portal
	_exit_camera->set_global_transform(real_to_exit_transform(
		_main_camera->get_global_transform()));

	// Get the four X, Y corners of the scaled entrance portal bounding box clamped to Z=0 (portal surface)
	// relative to  the exit portal. The entrance portal bounding box is used since the entrance portal mesh
	// does not need to match the exit portal mesh.
	auto corner_1 = _exit_portal->to_global(Vector3(_mesh_aabb.position.x, _mesh_aabb.position.y, 0) * _exit_portal_scale);
	auto corner_2 = _exit_portal->to_global(Vector3(_mesh_aabb.position.x + _mesh_aabb.size.x, _mesh_aabb.position.y, 0) * _exit_portal_scale);
	auto corner_3 = _exit_portal->to_global(Vector3(_mesh_aabb.position.x + _mesh_aabb.size.x, _mesh_aabb.position.y + _mesh_aabb.size.y, 0) * _exit_portal_scale);
	auto corner_4 = _exit_portal->to_global(Vector3(_mesh_aabb.position.x, _mesh_aabb.position.y + _mesh_aabb.size.y, 0) * _exit_portal_scale);

	// Calculate the distance along the exit camera forward vector at which each of the portal corners projects
	auto camera_forward = -_exit_camera->get_global_transform().get_basis().get_column(2 /* z */).normalized();

	auto d_1 = (corner_1 - _exit_camera->get_global_position()).dot(camera_forward);
	auto d_2 = (corner_2 - _exit_camera->get_global_position()).dot(camera_forward);
	auto d_3 = (corner_3 - _exit_camera->get_global_position()).dot(camera_forward);
	auto d_4 = (corner_4 - _exit_camera->get_global_position()).dot(camera_forward);

	// The near clip distance is the shortest distance which still contains all the corners
	_exit_camera->set_near(MAX(EXIT_CAMERA_NEAR_MIN, MIN(MIN(d_1, d_2), MIN(d_3, d_4)) - _exit_near_subtract));
	_exit_camera->set_far(_main_camera->get_far());
	_exit_camera->set_fov(_main_camera->get_fov());
	_exit_camera->set_keep_aspect_mode(_main_camera->get_keep_aspect_mode());
}

// Return a new Transform3D relative to the exit portal based on the real Transform3D relative to this portal.
Transform3D Portal::real_to_exit_transform(Transform3D real)
{
	// Convert from global space to local space at the entrance (this) portal
	auto local = get_global_transform().affine_inverse() * real;
	// Compensate for any scale the entrance portal may have
	auto unscaled = local.scaled(get_global_transform().basis.get_scale());
	// Flip it (the portal always flips the view 180 degrees)
	auto flipped = unscaled.rotated(Vector3(0, 1, 0) /* up */, Math_PI);
	// Apply any scale the exit portal may have (and apply custom exit scale)
	auto exit_scale_vector = _exit_portal->get_global_transform().basis.get_scale();
	auto scaled_at_exit = flipped.scaled(Vector3(1, 1, 1) /* one */ / exit_scale_vector * _exit_portal_scale);
	// Convert from local space at the exit portal to global space
	auto local_at_exit = _exit_portal->get_global_transform() * scaled_at_exit;
	return local_at_exit;
}
