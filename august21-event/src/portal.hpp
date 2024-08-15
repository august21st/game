#pragma once
#include <godot_cpp/classes/shader_material.hpp>
#include <godot_cpp/variant/node_path.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/classes/mesh_instance3d.hpp>
#include <godot_cpp/classes/camera3d.hpp>
#include <godot_cpp/classes/environment.hpp>
#include <godot_cpp/classes/sub_viewport.hpp>
#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/classes/environment.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/area3d.hpp>

#include "client.hpp"
#include "godot_cpp/classes/node3d.hpp"

using namespace godot;

class Portal : public MeshInstance3D {
    GDCLASS(Portal, MeshInstance3D);

private:
	Engine* _engine;
	ResourceLoader* _resource_loader;
	Client* _client;
	AABB _mesh_aabb;
	SubViewport* _viewport;
	Camera3D* _exit_camera;
	Ref<ShaderMaterial> _material_override;
	float _seconds_until_resize = 0;
	void create_viewport();
	void _on_viewport_size_changed();
	Transform3D real_to_exit_transform(Transform3D real);
	const float RESIZE_THROTTLE_SECONDS = 0.1f;
	const float EXIT_CAMERA_NEAR_MIN = 0.01f;

	// @export properties
	// Viewport covers entire screen, clipped by portal AABB (Set to -1 for native resolution)
	int _viewport_vertical_resolution = 512;
	// Disable distance viewports
	float _disable_viewport_distance = 11.0f;
	// Destroy out of range viewports to conserve VRAM
	bool _destroy_disabled_viewport = true;
	float _fade_out_distance_max = 10.0f;
	float _fade_out_distance_min = 8.0f;
	Color _fade_out_colour = Color(1.0f, 1.0f, 1.0f, 1.0f);
	// Relative scale of the exit side portal
	float _exit_portal_scale = 1.0f;
	// Subtracted from exit camera near clipping plane (clipping issue fix)
	float _exit_near_subtract = 0.05f;
	// Main rendering camera that portal is drawn according to (Set to nullptr for default camera)
	NodePath _main_camera_path;
	Camera3D* _main_camera;
	// Exit camera environment (Set to nullptr for default environment)
	NodePath _exit_environment_override_path;
	Environment* _exit_environment_override;
	// The exit portal. Leave unset to use this portal as an exit only.
	NodePath _exit_portal_path;
	Portal* _exit_portal;
	// Collision area which will trigger a teleport for any physics object that enters
	NodePath _entrance_area_path;
	Area3D* _entrance_area;
	bool ready_and_connected();
	void _on_current_scene_ready();
	void set_enabled(bool enable);
	void _on_entrance_area_body_entered(Node3D* body);

protected:
    static void _bind_methods();

public:
	Portal();
	~Portal();
    void _ready() override;
    void _process(double delta) override;
    int get_viewport_vertical_resolution();
	void set_viewport_vertical_resolution(int value);
	float get_disable_viewport_distance();
	void set_disable_viewport_distance(float value);
	bool get_destroy_disabled_viewport();
	void set_destroy_disabled_viewport(bool value);
	float get_fade_out_distance_max();
	void set_fade_out_distance_max(float value);
	float get_fade_out_distance_min();
	void set_fade_out_distance_min(float value);
	Color get_fade_out_colour();
	void set_fade_out_colour(Color value);
	float get_exit_portal_scale();
	void set_exit_portal_scale(float value);
	float get_exit_near_subtract();
	void set_exit_near_subtract(float value);
	/* Camera3D* */ NodePath get_main_camera_override();
	void set_main_camera_override(const NodePath& path);
	/* Environment* */ NodePath get_exit_environment_override();
	void set_exit_environment_override(const NodePath& path);
	/* Portal* */ NodePath get_exit_portal();
	void set_exit_portal(const NodePath& path);
	/* Area3D* */ NodePath get_entrance_area();
	void set_entrance_area(const NodePath& path);
};
