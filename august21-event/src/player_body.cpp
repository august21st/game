#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/input.hpp>
#include <godot_cpp/classes/input_event.hpp>
#include <godot_cpp/classes/input_event_mouse_motion.hpp>
#include <godot_cpp/classes/input_event_mouse_button.hpp>
#include <godot_cpp/classes/input_event_screen_drag.hpp>
#include <godot_cpp/classes/input_event_action.hpp>
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/variant/vector3.hpp>
#include <godot_cpp/classes/camera3d.hpp>
#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/classes/panel.hpp>
#include <godot_cpp/classes/button.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/classes/rich_text_label.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/classes/animation_player.hpp>
#include <godot_cpp/core/math.hpp>
#include <godot_cpp/variant/callable.hpp>
#include <godot_cpp/classes/performance.hpp>
#include <godot_cpp/classes/texture_rect.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/vector2.hpp>

#include "player_body.hpp"

using namespace godot;

const float MAX_SPEED = 6.0f;
const float MAX_CLIMB_SPEED = 5.0f;
const float ACCELERATION = 12.0f;
const float DECELERATION = 10.0f;
const float JUMP_SPEED = 5.0f;
const float MOUSE_SENSITIVITY = 0.001f;
const float FALL_DAMAGE_THRESHOLD = 1.0f;
const int DEFAULT_HEALTH = 100;
const float STILL_THRESHOLD  = 0.1;
const float PI = 3.14159265358979f;

static inline double round_decimal(double value, int places)
{
	return Math::round(value*Math::pow(10.0, places)) / Math::pow(10.0, places);
}

// min: top left corner, max: bottom left corner, object can move within a circular area within the rectangle
static Vector2 circular_clamp(const Vector2& vector, const Vector2& min, const Vector2& max)
{
    Vector2 center = (min + max) * 0.5f;
    float radius = std::min((max.x - min.x) * 0.5f, (max.y - min.y) * 0.5f);
    Vector2 to_vector = vector - center;
    float length = to_vector.length();

    if (length > radius) {
        return center + to_vector.normalized() * radius;
    }

    return vector;
}

PlayerBody::PlayerBody()
{
}

PlayerBody::~PlayerBody()
{
}

void PlayerBody::set_stats_enabled(bool enable)
{
	_stats_enabled = enable;
	_stats_label->set_visible(enable);
}

void PlayerBody::update_stats()
{
	auto fps = _engine->get_frames_per_second();
	auto frame_time = _performance->get_monitor(Performance::Monitor::TIME_FPS);
	auto object_count = _performance->get_monitor(Performance::Monitor::OBJECT_COUNT);
	auto rendered_objects_count = _performance->get_monitor(Performance::Monitor::RENDER_TOTAL_OBJECTS_IN_FRAME);
	auto memory_static = _performance->get_monitor(Performance::Monitor::MEMORY_STATIC);
	auto memory_static_unit = "B";
	if (memory_static > 1'000'000) {
		memory_static_unit = "MB";
		memory_static = round_decimal(memory_static / 1'000'000, 2);
	}
	else if (memory_static > 1'000) {
		memory_static_unit = "KB";
		memory_static = Math::floor(memory_static / 1'000);
	}
	auto stats_string = String("fps: {0}\nframe time: {1}\nscene objects: {2}\nRendered objects: {3}\nStatic memory usage: {4}{5}\n")
		.format(Array::make(fps, frame_time, object_count, rendered_objects_count, memory_static, memory_static_unit));
	_stats_label->set_text(stats_string);
}

void PlayerBody::_bind_methods()
{
	ClassDB::bind_method(D_METHOD("_on_revive_pressed"),
		&PlayerBody::_on_revive_pressed);
	ClassDB::bind_method(D_METHOD("_on_thumbstick_button_down"),
		&PlayerBody::_on_thumbstick_button_down);
	ClassDB::bind_method(D_METHOD("_on_thumbstick_button_up"),
		&PlayerBody::_on_thumbstick_button_up);
	ClassDB::bind_method(D_METHOD("_on_jump_button_down"),
		&PlayerBody::_on_jump_button_down);
	ClassDB::bind_method(D_METHOD("_on_jump_button_up"),
		&PlayerBody::_on_jump_button_up);
}

void PlayerBody::_ready()
{
	_engine = Engine::get_singleton();
	if (_engine->is_editor_hint()) {
		return;
	}

	_performance = Performance::get_singleton();
	_player_input = Input::get_singleton();
	_project_settings = ProjectSettings::get_singleton();
	_gravity = _project_settings->get_setting("physics/3d/default_gravity");
	_camera = get_node<Camera3D>("%PlayerCamera");
	_camera_animation_player = get_node<AnimationPlayer>("%PlayerCameraAnimationPlayer");
	_camera_pivot = get_node<Node3D>("%PlayerCameraPivot");
	_death_panel = get_node<Control>("%DeathPanel");
	_death_title_label = get_node<Label>("%DeathTitleLabel");
	_death_message_label = get_node<RichTextLabel>("%DeathMessageLabel");
	_stats_label = get_node<Label>("%StatsLabel");
	_grab_button = get_node<Button>("%ReviveButton");
	_revive_button = get_node<Button>("%ReviveButton");
	_revive_button->connect("pressed", Callable(this, "_on_revive_pressed"));
	_thumbstick_panel = get_node<TextureRect>("%ThumbstickPanel");
	_thumbstick_button = get_node<Button>("%ThumbstickButton");
	_thumbstick_button->connect("button_down", Callable(this, "_on_thumbstick_button_down"));
	_thumbstick_button->connect("button_up", Callable(this, "_on_thumbstick_button_up"));
	_dragging_thumbstick = false;
	_thumbstick_direction = Vector2(0, 0);
	_jump_button = get_node<Button>("%JumpButton");
	_jump_button->connect("button_down", Callable(this, "_on_jump_button_down"));
	_jump_button->connect("button_up", Callable(this, "_on_jump_button_up"));
	_jump_pressed = false;
	_velocity = Vector3(0, 0, 0);
	_health = DEFAULT_HEALTH;
	_death_panel->set_visible(false);
	_climbing = false;
	spawn_position = get_position();
	set_stats_enabled(false);
	set_process_input(true);
	set_process_unhandled_input(true);
	set_process(true);
}

void PlayerBody::_input(const Ref<InputEvent> &event)
{
	if (_engine->is_editor_hint()) {
		return;
	}

	if (event->is_class("InputEventMouseMotion")) {
		const Ref<InputEventMouseMotion> event_mouse_motion = event;
		auto mouse_relative = event_mouse_motion->get_relative();

		if (_dragging_thumbstick) {
			auto panel_position = _thumbstick_panel->get_global_position();
			auto mouse_panel_position = event_mouse_motion->get_position() - panel_position;
			auto panel_size = _thumbstick_panel->get_global_rect().get_size();
			auto new_position = circular_clamp(mouse_panel_position, Vector2(0, 0), panel_size);
			_thumbstick_button->set_position(new_position - _thumbstick_button->get_size() / 2.0f);
			_thumbstick_direction = (new_position * 2.0f - panel_size) / panel_size;
		}
		else {
			auto body_rotation = get_rotation();
			body_rotation.y -= mouse_relative.x * MOUSE_SENSITIVITY;
			set_rotation(body_rotation);

			auto camera_rotation = _camera_pivot->get_rotation();
			camera_rotation.x -= mouse_relative.y * MOUSE_SENSITIVITY;
			camera_rotation.x = Math::clamp(camera_rotation.x, -PI / 2.0f, PI / 2.0f);
			_camera_pivot->set_rotation(camera_rotation);
		}
	}
	else if (event->is_class("InputEventScreenDrag")) {
		const Ref<InputEventScreenDrag> event_screen_drag = event;
		auto relative_drag = event_screen_drag->get_relative();
	}
	else if (event->is_action_pressed("unfocus")) {
		// Release pointer lock
		_player_input->set_mouse_mode(godot::Input::MOUSE_MODE_VISIBLE);
	}
	else if (event->is_action_pressed("toggle_stats")) {
		set_stats_enabled(!_stats_enabled);
	}
}

void PlayerBody::_unhandled_input(const Ref<InputEvent> &event)
{
	if (_engine->is_editor_hint()) {
		return;
	}

	if (event->is_class("InputEventMouseButton")) {
		const Ref<InputEventMouseButton> event_mouse_button = event;
		_player_input->set_mouse_mode(godot::Input::MOUSE_MODE_CAPTURED);
	}
}

void PlayerBody::_physics_process(double delta)
{
	if (_engine->is_editor_hint()) {
		return;
	}

	// Movement
	auto direction = Vector3(0, 0, 0);
	if (_dragging_thumbstick) {
		direction.x = _thumbstick_direction.x;
		direction.z = _thumbstick_direction.y;
	}
	else {
		direction.x = _player_input->get_action_strength("move_right") - _player_input->get_action_strength("move_left");
		direction.z = _player_input->get_action_strength("move_back") - _player_input->get_action_strength("move_forward");
		direction.normalize();
	}

	// Move in direction we are looking
	direction.rotate(Vector3(0, 1, 0), get_rotation().y);

	if (!_climbing) {
		// Gravity
		_velocity.y -= delta * _gravity;

		auto horizontal_velocity = _velocity;
		horizontal_velocity.y = 0;
		auto target_velocity = direction * MAX_SPEED;

		float acceleration;
		if (direction.dot(horizontal_velocity) > 0) {
			acceleration = ACCELERATION;
		}
		else {
			acceleration = DECELERATION;
		}

		// Apply horizontal velocity
		horizontal_velocity = horizontal_velocity.lerp(target_velocity, acceleration * delta);
		_velocity.x = horizontal_velocity.x;
		_velocity.z = horizontal_velocity.z;

		// Jump
		if (is_on_floor()) {
			if (_player_input->is_action_pressed("jump") || _jump_pressed) {
				_velocity.y = JUMP_SPEED;
			}
			else if (_velocity.y != 0) {
				auto fall_speed = (int) Math::floor(Math::floor(_velocity.y));
				if (fall_speed > FALL_DAMAGE_THRESHOLD) {
					take_damage(fall_speed * 2);
				}

				_velocity.y = 0;
			}
		}
	}
	else {
		// Swizzle forward motion & direction to upward motion & direction

		/*auto horizontal_velocity = _velocity;
		horizontal_velocity.y = 0;
		auto target_velocity = direction * MAX_SPEED;

		float acceleration;
		if (direction.dot(horizontal_velocity) > 0) {
			acceleration = ACCELERATION;
		}
		else {
			acceleration = DECELERATION;
		}
		horizontal_velocity = horizontal_velocity.lerp(target, ACCELERATION * delta);*/

	}

	// Camera bob
	if (Math::abs(_velocity.length()) > STILL_THRESHOLD) {
		_camera_animation_player->play("player_camera_bob");
	}
	else {
		_camera_animation_player->stop(true);
	}

	set_velocity(_velocity);
	move_and_slide();
}

void PlayerBody::_process(double delta)
{
	if (_engine->is_editor_hint()) {
		return;
	}
	if (_stats_enabled) {
		update_stats();
	}
}

void PlayerBody::_on_revive_pressed()
{
	PlayerBody::respawn(spawn_position);
}

void PlayerBody::_on_thumbstick_button_down()
{
	_dragging_thumbstick = true;
}

void PlayerBody::_on_thumbstick_button_up()
{
	_dragging_thumbstick = false;
	_thumbstick_direction = Vector2(0, 0);
	_thumbstick_button->set_position((_thumbstick_panel->get_global_rect().get_size() - _thumbstick_button->get_size()) / 2.0f);
	_thumbstick_button->set_focus_mode(godot::Control::FOCUS_NONE);
}

void PlayerBody::_on_jump_button_down()
{
	_jump_pressed = true;
}

void PlayerBody::_on_jump_button_up()
{
	_jump_pressed = false;
}

void PlayerBody::take_damage(int damage)
{
	_health -= damage;
	if (_health < 0) {
		die();
	}
}

void PlayerBody::respawn(Vector3 position)
{
	set_position(position);
	_camera_pivot->set_rotation(Vector3(90, 0, 0));
	set_rotation(Vector3(0, 0, 0));
	_player_input->set_mouse_mode(godot::Input::MOUSE_MODE_CAPTURED);
	_death_panel->set_visible(false);
	_health = DEFAULT_HEALTH;
}

void PlayerBody::die(String death_title, String death_mesage)
{
	_player_input->set_mouse_mode(godot::Input::MOUSE_MODE_VISIBLE);
	_death_panel->set_visible(true);
	_death_title_label->set_text(death_title);
	_death_message_label->set_text(death_mesage);
}

void PlayerBody::set_climbing(bool climbing)
{
	_climbing = climbing;
}
