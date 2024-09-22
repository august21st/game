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
#include <godot_cpp/classes/skeleton3d.hpp>
#include <godot_cpp/classes/margin_container.hpp>
#include <godot_cpp/classes/nine_patch_rect.hpp>
#include <godot_cpp/classes/rigid_body3d.hpp>
#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/classes/h_box_container.hpp>
#include <godot_cpp/classes/texture2d.hpp>
#include <godot_cpp/classes/mesh.hpp>
#include <dataproto_cpp/dataproto.hpp>

#include "client.hpp"
#include "player_body.hpp"
#include "entity_player_base.hpp"
#include "entity_item_base.hpp"
#include "network_shared.hpp"
#include "client.hpp"
#include "node_shared.hpp"

using namespace godot;
using namespace NetworkShared;
using namespace NodeShared;

PlayerBody::PlayerBody() : _client(nullptr)
{
}

PlayerBody::~PlayerBody()
{
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
	_client = get_tree()->get_root()->get_node<Client>("/root/GlobalClient");
	if (_client == nullptr) {
		UtilityFunctions::printerr("Could not get client: autoload singleton was null");
		return;
	}
	_client->connect("packet_received", Callable(this, "_on_packet_received"));

	_resource_loader = ResourceLoader::get_singleton();
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
	_animation_player = _player_model->get_node<AnimationPlayer>("AnimationPlayer");
	_inventory_box = get_node<HBoxContainer>("%InventoryBox");
	_inventory_selector = get_node<NinePatchRect>("%InventorySelector");
	_inventory_selector_label = get_node<Label>("%InventorySelectorLabel");
	_velocity = Vector3(0, 0, 0);
	_health = DEFAULT_HEALTH;
	_is_dead = false;
	_death_panel->set_visible(false);
	_climbing = false;
	_spawn_position = Vector3(0, 0, 0);
	_update_tick = 0;
	_inventory = { };
	_hovered_item_entity = nullptr;
	set_inventory_current(-1);

	auto item_outline_resource = _resource_loader->load("res://assets/item_outline_material.tres");
	if (item_outline_resource.is_valid() && item_outline_resource->is_class("ShaderMaterial")) {
		_item_outline_material = item_outline_resource;
	}
	else {
		UtilityFunctions::print("Failed to load outline material: Resource was not a ShaderMaterial");
	}

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
	else if (event->is_action_pressed("hotbar_first")) {
		set_inventory_current(0);
	}
	else if (event->is_action_pressed("hotbar_second")) {
		set_inventory_current(1);
	}
	else if (event->is_action_pressed("hotbar_third")) {
		set_inventory_current(2);
	}
	else if (event->is_action_pressed("hotbar_fourth")) {
		set_inventory_current(3);
	}
	else if (event->is_action_pressed("hotbar_fifth")) {
		set_inventory_current(4);
	}
	else if (event->is_action_pressed("hotbar_sixth")) {
		set_inventory_current(5);
	}
	else if (event->is_action_pressed("hotbar_seventh")) {
		set_inventory_current(6);
	}
	else if (event->is_action_pressed("hotbar_eigth")) {
		set_inventory_current(7);
	}
	else if (event->is_action_pressed("hotbar_ninth")) {
		set_inventory_current(8);
	}
	else if (event->is_action_pressed("hotbar_tenth")) {
		set_inventory_current(9);
	}
}

void PlayerBody::set_mesh_next_pass_recursive(Node* root_node, Ref<Material> material)
{
	auto meshes = root_node->find_children("*", "MeshInstance3D");
	for (auto i = 0; i < meshes.size(); i++) {
		auto mesh_instance = Object::cast_to<MeshInstance3D>(meshes[i]);
		if (mesh_instance == nullptr) {
			continue;
		}
		auto surface_count = mesh_instance->get_mesh()->get_surface_count();
		for (auto j = 0; j < surface_count; j++) {
			auto surface_material = mesh_instance->get_mesh()->surface_get_material(j);
			surface_material->set_next_pass(material);
		}
	}
}

void PlayerBody::_unhandled_input(const Ref<InputEvent> &event)
{
	// Draw outline around items
	auto collider_object = _grab_ray->get_collider();
	auto item_entity = Object::cast_to<EntityItemBase>(collider_object);
	if (item_entity != nullptr && item_entity->can_grab() && _hovered_item_entity != item_entity) {
		// Clear surface next pass from previous item
		if (_hovered_item_entity != nullptr) {
			auto hovered_item_node = _hovered_item_entity->get_item_node();
			if (hovered_item_node != nullptr) {
				set_mesh_next_pass_recursive(hovered_item_node, nullptr);
			}
		}

		// Apply surface next pass to new item
		auto item_node = item_entity->get_item_node();
		if (item_node != nullptr) {
			set_mesh_next_pass_recursive(item_node, _item_outline_material);
		}
		_hovered_item_entity = item_entity;
	}
	else if (item_entity == nullptr && _hovered_item_entity != nullptr) {
		auto hovered_item_node = _hovered_item_entity->get_item_node();
		set_mesh_next_pass_recursive(hovered_item_node, nullptr);
		// Hovering nothing
		_hovered_item_entity = nullptr;
	}

	// Process item clicks
	if (event->is_class("InputEventMouseButton")) {
		const Ref<InputEventMouseButton> event_mouse_button = event;
		_player_input->set_mouse_mode(godot::Input::MOUSE_MODE_CAPTURED);

		switch (event_mouse_button->get_button_index()) {
			case MouseButton::MOUSE_BUTTON_LEFT:  {
				Ref<Texture2D> thumbnail = nullptr;
				// Try to obtain grab of item (if not claimed by something else)
				// and move the entity item into our inventory (ACTION_GRAB)
				if (_inventory.size() < MAX_INVENTORY_SIZE && item_entity != nullptr && item_entity->try_grab()) {
					add_child(item_entity);
					_inventory.push_back(item_entity);
					_inventory_current = _inventory.size() - 1;
					auto item_node = item_entity->get_item_node();
					if (item_node != nullptr) {
						set_mesh_next_pass_recursive(item_node, nullptr);
					}

					// Gui
					Node* inventory_item = nullptr;
					auto load_error = load_scene("res://scenes/inventory_item.tscn", &inventory_item);
					if (load_error == Error::OK) {
						auto thumbnail_path = item_entity->get_thumbnail_path();
						if (!thumbnail_path.is_empty()) {
							auto thumbnail_resource = _resource_loader->load(thumbnail_path);
							if (thumbnail_resource->is_class("Texture2D")) {
								thumbnail = thumbnail_resource;
							}
							else {
								UtilityFunctions::print("Failed to read from thumbnail path ",
									thumbnail_path, ": resource was not a Texture2D");
							}
						}
						if (thumbnail.is_valid()) {
							auto item_image = inventory_item->get_node<TextureRect>("%ItemImage");
							item_image->set_texture(thumbnail);
						}
						else {
							auto item_label = inventory_item->get_node<Label>("%ItemLabel");
							item_label->set_text(item_entity->get_name());
						}
					}
					else {
						UtilityFunctions::print("Failed to add inventory item: Loading inventory item scene failed with ", load_error);
					}
					_inventory_box->add_child(inventory_item);

					// Notify server
					auto grab_packet = BufWriter();
					grab_packet.u8(to_uint8(ClientPacket::ACTION_GRAB));
					auto entity_id = _client->get_entity_id(item_entity);
					if (entity_id != -1) {
						grab_packet.u32(entity_id);
					}
					_client->send(grab_packet);
				}
				break;
			}
			case MouseButton::MOUSE_BUTTON_WHEEL_UP: {
				scroll_inventory_current(1);
				break;
			}
			case MouseButton::MOUSE_BUTTON_WHEEL_DOWN: {
				scroll_inventory_current(-1);
				break;
			}
			default:
				break;
		}
	}
}

void PlayerBody::_physics_process(double delta)
{
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

	// Gravity
	_velocity.y -= delta * _gravity;

	if (!_climbing) {
		// Copy direction and rotate to move in direction we are looking
		auto walk_direction = direction.rotated(Vector3(0, 1, 0), get_rotation().y);
		auto horizontal_velocity = _velocity;
		horizontal_velocity.y = 0;
		auto target_velocity = walk_direction * MAX_SPEED;

		float acceleration;
		if (walk_direction.dot(horizontal_velocity) > 0) {
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
		// z-direction is into the area
		auto climb_direction = Vector3(direction.x, direction.y, direction.z);
		climb_direction.y = direction.z;
		if (_player_input->is_action_pressed("jump") || _jump_pressed) {
			climb_direction.y = 1.0f;
		}

		if (climb_direction.y > 0) {
			// Climb up
			_velocity.y = climb_direction.y * MAX_CLIMB_SPEED;
			climb_direction.z = 0.0f;
		}
		else {
			// Descend (& move backward off ladder)
			_velocity.y = Math::max(_velocity.y, -MAX_CLIMB_SPEED);
		}

		// Rotate direction to move in direction we are looking
		climb_direction.rotate(Vector3(0, 1, 0), get_rotation().y);

		_velocity.x = climb_direction.x * MAX_CLIMB_SPEED;
		_velocity.z = climb_direction.z * MAX_CLIMB_SPEED;
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

	// Update player on server every 20 ticks on move
	auto current_position = get_global_position();
	auto current_rotation = get_global_rotation();
	auto moved = current_position.distance_to(_last_packet_posiiton) > 0.1f
		|| current_rotation.distance_to(_last_packet_rotation) > 0.1f;
	if (_update_tick % 3 == 0 && moved) {
		auto update_packet = BufWriter();
		update_packet.u8(to_uint8(ClientPacket::UPDATE_MOVEMENT));
		auto phase_scene = _client->get_current_phase_scene();
		auto phase_scene_utf8 = phase_scene.utf8().get_data();
		update_packet.str(phase_scene_utf8);
		auto position = get_global_position();
		write_vector3(update_packet, position);
		auto velocity = _velocity;
		write_vector3(update_packet, velocity);
		auto rotation = get_global_rotation();
		write_vector3(update_packet, rotation);
		// TODO: Implement current_animation
		auto current_animation = String("idle");
		auto current_animation_utf8 = current_animation.utf8().get_data();
		update_packet.str(current_animation_utf8);
		update_packet.u32(_health);
		_client->send(update_packet);

		_last_packet_posiiton = current_position;
		_last_packet_rotation = current_rotation;
	}
	_update_tick++;
}

void PlayerBody::_process(double delta)
{
	// Process inventory updates
	// TODO: Refactor as much of this out into methods that do not need to run each frame
	auto health_label = String("Health: {0}").format(Array::make(_health));
	_health_label->set_text(health_label);
	// 3D
	for (auto i = 0; i < _inventory.size(); i++) {
		auto inventory_item = _inventory[i];
		if (i == _inventory_current) {
			inventory_item->set_visible(true);
			_animation_player->play(inventory_item->get_hold_animation(), 0.5f);

			auto hand_bone = _skeleton->find_bone("DEF-HandR");
			auto hand_transform = _skeleton->get_bone_global_pose(hand_bone);
			//inventory_item->set_position(hand_transform.get_origin());
			//auto hand_rotation = hand_transform.basis.get_euler();
			//inventory_item->set_rotation(hand_rotation);
			// DEBUG: Item positions TODO: Fix this!
			inventory_item->set_position(Vector3(-0.19, 1.17f, 0.23f));
			inventory_item->set_rotation(Vector3(0.0f, 0.0f, 0.0f));
		}
		else {
			inventory_item->set_visible(false);
		}
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
	auto code = static_cast<ServerPacket>(packet.u8());
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
	damage_packet.u8(to_uint8(ClientPacket::ACTION_TAKE_DAMAGE));
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
	chat_packet.u8(to_uint8(ClientPacket::ACTION_CHAT_MESSAGE));
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
	_camera_pivot->set_rotation(Vector3(0, 0, 0));
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

int PlayerBody::scroll_inventory_current(int by)
{
	int selected = -1;
	if (_inventory.size() != 0) {
		auto selected = (_inventory_current + by) % _inventory.size();
		if (selected < 0) {
			selected = _inventory.size() + selected;
		}
	}
	set_inventory_current(selected);
	return selected;
}

void PlayerBody::set_inventory_current(int index) {
	_inventory_current = Math::min(index, _inventory.size() - 1);

	// UI
	if (_inventory_current == -1) {
		_inventory_selector->set_visible(false);
		_inventory_selector->set_modulate(Color(1.0f, 1.0f, 1.0f, 0.0f));
	}
	else {
		_inventory_selector->set_visible(true);
		auto item_node = Object::cast_to<Control>(_inventory_box->get_child(_inventory_current));
		if (item_node != nullptr) {
			auto item_position = item_node->get_global_position();
			auto item_size = item_node->get_size();

			auto selector_tween = create_tween();
			selector_tween->tween_property(_inventory_selector, "global_position", item_position - Vector2(8, 8), 0.2)
				->set_trans(Tween::TransitionType::TRANS_QUAD)
				->set_ease(Tween::EASE_IN);
			selector_tween->parallel()->tween_property(_inventory_selector, "size", item_size + Vector2(16, 16), 0.2)
				->set_trans(Tween::TransitionType::TRANS_QUAD)
				->set_ease(Tween::EASE_IN);
			selector_tween->parallel()->tween_property(_inventory_selector, "modulate", Color(1.0f, 1.0f, 1.0f, 1.0f), 0.2)
				->set_trans(Tween::TransitionType::TRANS_QUAD)
				->set_ease(Tween::EASE_IN);
			_inventory_selector_label->set_text(String("({0})").format(Array::make(_inventory_current + 1)));
		}
	}
}
