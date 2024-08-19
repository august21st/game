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
#include <godot_cpp/classes/line_edit.hpp>
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
#include <godot_cpp/classes/tween.hpp>
#include <godot_cpp/classes/property_tweener.hpp>
#include <godot_cpp/classes/callback_tweener.hpp>
#include <godot_cpp/classes/viewport.hpp>
#include <godot_cpp/classes/window.hpp>
#include <godot_cpp/classes/web_socket_peer.hpp>
#include <godot_cpp/classes/v_box_container.hpp>
#include <godot_cpp/variant/packed_byte_array.hpp>
#include <godot_cpp/classes/ray_cast3d.hpp>
#include <godot_cpp/classes/global_constants.hpp>
#include <dataproto_cpp/dataproto.hpp>
#include <godot_cpp/classes/skeleton3d.hpp>

#include "client.hpp"
#include "player_body.hpp"
#include "entity_player_base.hpp"
#include "entity_item_base.hpp"
#include "godot_cpp/classes/h_box_container.hpp"
#include "godot_cpp/classes/input_event_mouse.hpp"
#include "godot_cpp/classes/rigid_body3d.hpp"
#include "network_shared.hpp"
#include "client.hpp"
#include "node_shared.hpp"

using namespace godot;
using namespace NetworkShared;
using namespace NodeShared;

const float MAX_SPEED = 6.0f;
const float MAX_CLIMB_SPEED = 5.0f;
const float ACCELERATION = 12.0f;
const float DECELERATION = 10.0f;
const float JUMP_SPEED = 5.0f;
const float MOUSE_SENSITIVITY = 0.001f;
const float FALL_DAMAGE_THRESHOLD = 5.2f;
const float FALL_DAMAGE_MIN = 4.0f;
const int DEFAULT_HEALTH = 100;
const float STILL_THRESHOLD  = 0.1;
const float PI = 3.14159265358979f;

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

PlayerBody::PlayerBody() : _client(nullptr)
{
}

PlayerBody::~PlayerBody()
{
}

void PlayerBody::update_hotbar()
{
	auto health_label = String("Health: {0}").format(Array::make(_health));
	_health_label->set_text(health_label);

	_inventory_selector->set_visible(_inventory_current == -1);
	auto inventory_position = _inventory_box->get_position();
	auto inventory_size = _inventory_box->get_size();
	_inventory_selector->set_position(Vector2(
		inventory_position.x + (_inventory_current / (float) MAX_INVENTORY_SIZE) * inventory_size.x,
		inventory_position.y + 8));
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
	ClassDB::bind_method(D_METHOD("_on_chat_button_pressed"),
		&PlayerBody::_on_chat_button_pressed);
	ClassDB::bind_method(D_METHOD("_on_chat_close_button_pressed"),
		&PlayerBody::_on_chat_close_button_pressed);
	ClassDB::bind_method(D_METHOD("_on_chat_close_tween_completed"),
		&PlayerBody::_on_chat_close_tween_completed);
	ClassDB::bind_method(D_METHOD("_on_packet_received", "packed_packet"),
		&PlayerBody::_on_packet_received);
	ClassDB::bind_method(D_METHOD("_on_chat_submit"),
		&PlayerBody::_on_chat_submit);
}

void PlayerBody::_ready()
{
	_engine = Engine::get_singleton();
	if (_engine->is_editor_hint()) {
		return;
	}

	_client = get_tree()->get_root()->get_node<Client>("/root/GlobalClient");
	if (_client == nullptr) {
		UtilityFunctions::printerr("Could not get client: autoload singleton was null");
		return;
	}
	_client->connect("packet_received", Callable(this, "_on_packet_received"));

	_player_input = Input::get_singleton();
	_project_settings = ProjectSettings::get_singleton();
	_gravity = _project_settings->get_setting("physics/3d/default_gravity");
	_camera = get_node<Camera3D>("%PlayerCamera");
	_camera_animation_player = get_node<AnimationPlayer>("%PlayerCameraAnimationPlayer");
	_camera_pivot = get_node<Node3D>("%PlayerCameraPivot");
	_death_panel = get_node<Control>("%DeathPanel");
	_death_title_label = get_node<Label>("%DeathTitleLabel");
	_death_message_label = get_node<RichTextLabel>("%DeathMessageLabel");
	_health_label = get_node<Label>("%HealthLabel");
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
	_action_button = get_node<Button>("%ActionButton");
	_chat_button = get_node<Button>("%ChatButton");
	_chat_button->connect("pressed", Callable(this, "_on_chat_button_pressed"));
	_chat_close_button = get_node<Button>("%ChatCloseButton");
	_chat_close_button->connect("pressed", Callable(this, "_on_chat_close_button_pressed"));
	_chat_panel = get_node<Panel>("%ChatPanel");
	_chat_panel->set_visible(false);
	_chat_input = get_node<LineEdit>("%ChatInput");
	_chat_input->connect("text_submitted", Callable(this, "_on_chat_submit"));
	_chat_send_button = get_node<Button>("%ChatSendButton");
	_chat_send_button->connect("pressed", Callable(this, "_on_chat_submit"));
	_chat_messages_container = get_node<VBoxContainer>("%ChatMessagesContainer");
	_grab_ray = get_node<RayCast3D>("%PlayerGrabRay");
	_player_model = get_node<Node3D>("%PlayerModel");
	_skeleton = _player_model->get_node<Skeleton3D>("CharacterRig/Skeleton3D");
	_inventory_box = get_node<HBoxContainer>("%InventoryBox");
	_inventory_selector = get_node<Panel>("%InventorySelector");
	_velocity = Vector3(0, 0, 0);
	_health = DEFAULT_HEALTH;
	_is_dead = false;
	_death_panel->set_visible(false);
	_climbing = false;
	_spawn_position = Vector3(0, 0, 0);
	_update_tick = 0;
	_inventory = { };
	_inventory_current = -1;

	if (_client->get_presets_platform() != PresetsPlatform::MOBILE) {
		_thumbstick_panel->set_visible(false);
		_jump_button->set_visible(false);
		_action_button->set_visible(false);
	}

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
	else if (event->is_action_pressed("toggle_pause")) {
		close_chat();
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

		switch (event_mouse_button->get_button_index()) {
			case MouseButton::MOUSE_BUTTON_LEFT:  {
				auto collider_object = _grab_ray->get_collider();

				// Try to obtain grab of item (if not claimed by something else)
				// and move the entity item into our inventory (ACTION_GRAB)
				auto item_entity = Object::cast_to<EntityItemBase>(collider_object);
				if (_inventory.size() < MAX_INVENTORY_SIZE && item_entity != nullptr && item_entity->try_grab()) {
					add_child(item_entity);
					_inventory.push_back(item_entity);
					_inventory_current = _inventory.size() - 1;
					auto item_label = memnew(Label());
					item_label->set_text(item_entity->get_name());
					_inventory_box->add_child(item_label);
				}
				break;
			}
			case MouseButton::MOUSE_BUTTON_WHEEL_UP: {
				_inventory_current = (_inventory_current + 1) % _inventory.size();
				break;
			}
			case MouseButton::MOUSE_BUTTON_WHEEL_DOWN: {
				_inventory_current = (_inventory_current - 1) % _inventory.size();
				break;
			}
			default:
				break;
		}
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
		direction.x = -_thumbstick_direction.x;
		direction.z = -_thumbstick_direction.y;
	}
	else {
		direction.x = _player_input->get_action_strength("move_left")
			- _player_input->get_action_strength("move_right");
		direction.z = _player_input->get_action_strength("move_forward")
			- _player_input->get_action_strength("move_back");
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
			if (_velocity.y != 0) {
				auto fall_speed = (float) -_velocity.y;
				if (fall_speed > FALL_DAMAGE_THRESHOLD) {
					// (x=_velocity.y, y=fall_damage), a=FALL_DAMAGE_THRESHOLD, b=0, c=32, d=9.4, b=FALL_DAMAGE_MIN
					// f\left(x\right)=\frac{d-b}{\left(c-a\right)2}\left(x-a\right)^{2}+b
					auto fall_damage = (9.4f / Math::pow(32.0f - FALL_DAMAGE_THRESHOLD, 2.0f))
						* Math::pow(fall_speed - FALL_DAMAGE_THRESHOLD, 2.0f) + FALL_DAMAGE_MIN;
					take_damage(fall_damage);
				}

				_velocity.y = 0;
			}
			if (_player_input->is_action_pressed("jump") || _jump_pressed) {
				_velocity.y = JUMP_SPEED;
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

	// Update player on server
	if (_update_tick % 3 == 0) { // 20 tps
		auto update_packet = BufWriter();
		update_packet.u8(ClientPacket::UPDATE_MOVEMENT);
		auto phase_scene = _client->get_current_phase_scene();
		auto phase_scene_utf8 = phase_scene.utf8().get_data();
		update_packet.str(phase_scene_utf8);
		auto position = get_position();
		update_packet.f32(position.x);
		update_packet.f32(position.y);
		update_packet.f32(position.z);
		auto rotation = get_rotation();
		update_packet.f32(rotation.x);
		update_packet.f32(rotation.y);
		update_packet.f32(rotation.z);
		// TODO: Implement current_animation
		auto current_animation = String("walk");
		auto current_animation_utf8 = current_animation.utf8().get_data();
		update_packet.str(current_animation_utf8);
		update_packet.u32(_health);
		_client->send(update_packet);
	}
	_update_tick++;
}

void PlayerBody::_process(double delta)
{
	if (_engine->is_editor_hint()) {
		return;
	}

	// Inventory management
	update_hotbar();
	if (_inventory_current != -1) {
		auto inventory_item = _inventory[_inventory_current];
		auto hand_bone = _skeleton->find_bone("DEF-HandR");
		auto hand_transform = _skeleton->get_bone_global_pose(hand_bone);
		inventory_item->set_position(hand_transform.get_origin());
		inventory_item->set_rotation(hand_transform.basis.get_euler());
	}
}

void PlayerBody::set_spawn_position(Vector3 new_spawn_position)
{
	_spawn_position = new_spawn_position;
}

void PlayerBody::_on_revive_pressed()
{
	PlayerBody::respawn(_spawn_position);
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

void PlayerBody::open_chat()
{
	auto viewport_size = get_viewport()->get_window()->get_size();
	_chat_panel->set_position(Vector2(viewport_size.x, 0));
	_chat_panel->set_scale(Vector2(0.8, 0.8));

	_chat_panel->set_visible(true);
	_player_input->set_mouse_mode(Input::MOUSE_MODE_VISIBLE);
	_chat_input->grab_focus();

	auto chat_tween = create_tween();
	chat_tween->set_parallel(true);
	chat_tween->tween_property(_chat_panel, "position", Vector2(viewport_size.x - _chat_panel->get_size().x, 0), 0.2)
		->set_trans(Tween::TransitionType::TRANS_QUAD)
		->set_ease(Tween::EASE_OUT);
	chat_tween->tween_property(_chat_panel, "scale", Vector2(1.0, 1.0), 0.2)
		->set_trans(Tween::TransitionType::TRANS_QUAD)
		->set_ease(Tween::EASE_OUT);
	chat_tween->play();
}

void PlayerBody::close_chat()
{
	auto viewport_size = get_viewport()->get_window()->get_size();
	_chat_input->release_focus();

	auto chat_tween = create_tween();
	chat_tween->tween_property(_chat_panel, "position", Vector2(viewport_size.x, 0), 0.2)
		->set_trans(Tween::TransitionType::TRANS_QUAD)
		->set_ease(Tween::EASE_IN);
	chat_tween->parallel()->tween_property(_chat_panel, "scale", Vector2(0.8, 0.8), 0.2)
		->set_trans(Tween::TransitionType::TRANS_QUAD)
		->set_ease(Tween::EASE_IN);
	chat_tween->tween_callback(Callable(this, "_on_chat_close_tween_completed"));
	chat_tween->play();
}

void PlayerBody::_on_chat_close_tween_completed()
{
	_chat_panel->set_visible(false);
}


void PlayerBody::_on_packet_received(PackedByteArray packed_packet)
{
	auto packet = BufReader((char*) packed_packet.ptr(), packed_packet.size());
	uint8_t code = packet.u8();
	switch (code) {
		case ServerPacket::CHAT_MESSAGE: {
			auto player_id = packet.i32();
			String chat_name;
			if (player_id == 0) {
				chat_name = "SERVER@AUGUST21-EVENT";
			}
			else {
				auto player = _client->get_player(player_id);
				chat_name = player->get_chat_name();
				if (chat_name == "") {
					chat_name = "anon";
				}
			}
			auto message_str = (string) packet.str();
			auto message = String(message_str.c_str());

			auto message_label = memnew(RichTextLabel());
			message_label->set_use_bbcode(true);
			message_label->set_scroll_active(false);
			message_label->set_fit_content(true);
			message_label->set_text(String("[{0}] {1}")
				.format(Array::make(chat_name, message)));
			message_label->set_autowrap_mode(TextServer::AutowrapMode::AUTOWRAP_WORD_SMART);
			_chat_messages_container->add_child(message_label);
			break;
		}
	}
}

void PlayerBody::_on_chat_button_pressed()
{
	open_chat();
}

void PlayerBody::_on_chat_close_button_pressed()
{
	close_chat();
}

void PlayerBody::_on_chat_submit()
{
	if (_chat_input->get_text() != "") {
		send_chat(_chat_input->get_text());
		_chat_input->clear();
	}
}

void PlayerBody::take_damage(int damage)
{
	_health = Math::max(0, _health - damage);
	auto damage_packet = BufWriter();
	damage_packet.u8(ClientPacket::ACTION_TAKE_DAMAGE);
	damage_packet.u32(_health);
	_client->send(damage_packet);

	if (_health <= 0 && !_is_dead) {
		die();
	}
}

void PlayerBody::send_chat(String message)
{
	if (_client == nullptr) {
		UtilityFunctions::printerr("Could not send chat message: client was null");
		return;
	}

	auto chat_packet = BufWriter();
	chat_packet.u8(ClientPacket::ACTION_CHAT_MESSAGE);
	auto chat_message_utf8 = message.utf8().get_data();
	chat_packet.str(chat_message_utf8);
	_client->send(chat_packet);
}

void PlayerBody::set_health(int value)
{
	_health = value;
	if (_health <= 0 && !_is_dead) {
		die();
	}
}

void PlayerBody::respawn(Vector3 position)
{
	set_position(position);
	_camera_pivot->set_rotation(Vector3(90, 0, 0));
	set_rotation(Vector3(0, 0, 0));
	_player_input->set_mouse_mode(Input::MOUSE_MODE_CAPTURED);
	_death_panel->set_visible(false);
	_health = DEFAULT_HEALTH;
	_is_dead = false;
}

void PlayerBody::die(String death_title, String death_mesage)
{
	_health = 0;
	_is_dead = true;
	_player_input->set_mouse_mode(Input::MOUSE_MODE_VISIBLE);
	_death_panel->set_visible(true);
	_death_title_label->set_text(death_title);
	_death_message_label->set_text(death_mesage);
}

void PlayerBody::set_climbing(bool climbing)
{
	_climbing = climbing;
}
