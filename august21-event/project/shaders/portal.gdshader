/**
 * The contents of this file are based off the original work [godot-simple-portal-system]
 * (https://github.com/Donitzo/godot-simple-portal-system/blob/main/src/shaders/portal.gdshader),
 * by [Donitzo](https://github.com/Donitzo), released freely into the public domain under the
 * Creative Commons Zero v1.0 Universal license (https://creativecommons.org/publicdomain/zero/1.0/deed.en).
 *
 * ### Attribution:
 * - Original source by [Donitzo](https://github.com/Donitzo).
 * - Source port to C++ & adaptations by Zekiah-A & contributors.
 *
 * ### Disclaimer:
 * This adaptation is not endorsed by the original licensor and is provided without warranties,
 * this adaptation is NOT public domain, and is licensed under the project's primary license
 * ([GNU GPL 3.0](https://www.gnu.org/licenses/gpl-3.0.en.html)).
*/

shader_type spatial;
render_mode unshaded;

uniform float fade_out_distance_max = 10.0;
uniform float fade_out_distance_min = 8.0;
uniform vec4 fade_out_colour = vec4(0.0);

uniform sampler2D albedo:hint_default_black, source_color;

varying float pixel_distance;

void vertex() {
	// Calculate the world-space distance between the pixel and camera.
	// Pass the distance to the fragment shader using a varying attribute.
	vec3 world_position = (MODEL_MATRIX * vec4(VERTEX, 1.0)).xyz;
	vec3 camera_position = (INV_VIEW_MATRIX * vec4(0.0, 0.0, 0.0, 1.0)).xyz;

	pixel_distance = distance(world_position, camera_position);
}

void fragment() {
	// The portal colour is simply the screen-space color of the exit camera render target.
	// This is because the exit camera views the exit portal from the perspective of the player watching
	// the entrance portal, meaning the exit portal will occupy the same screen-space as the entrance portal.
	vec3 portal_colour = texture(albedo, SCREEN_UV).rgb;

	// Fade-out colour
	float t = smoothstep(fade_out_distance_min, fade_out_distance_max, pixel_distance);
	ALBEDO = mix(portal_colour, fade_out_colour.rgb, t);
}
