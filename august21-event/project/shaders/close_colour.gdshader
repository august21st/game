shader_type spatial;

uniform float radius : hint_range(0.0, 100.0) = 5.0;
uniform vec3 color_visible: source_color = vec3(1.0, 0.0, 0.0);
uniform vec3 color_transparent: source_color = vec3(0.0, 0.0, 0.0);

void fragment() {
    vec3 world_pos = (MODEL_MATRIX * vec4(VERTEX, 1.0)).xyz;
    vec3 camera_pos = CAMERA_POSITION_WORLD;
    float distance = length(world_pos - camera_pos);

    float factor = smoothstep(radius, radius + 1.0, distance);
    vec3 final_color = mix(color_visible, color_transparent, factor);

    ALBEDO = final_color;
    ALPHA = 1.0 - factor;
}
