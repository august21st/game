shader_type spatial;
render_mode unshaded;
render_mode fog_disabled;

uniform sampler2D image_texture : repeat_enable, filter_linear;
uniform sampler2D noise_texture : repeat_enable, filter_linear;
uniform float speed = 0.01;
uniform vec2 disp = vec2(1.0, 1.0);
uniform vec2 uv_scale = vec2(1.0, 1.0);
uniform float noise_weight = 0.05;
uniform float fade_radius = 0.5;
uniform float fade_softness = 0.2;
uniform vec3 add_colour : source_color = vec3(0.0, 0.0, 0.0);

void fragment() {
	vec2 uv = UV * uv_scale;

	// Noise sampling
	float noise_val = texture(noise_texture, uv + TIME * speed).r; 

	// Randomize displacement direction and amplitude
	float rand_x = sin(TIME * disp.x) * disp.x;
	float rand_y = cos(TIME * disp.y) * disp.y;

	uv.x += noise_val * noise_weight * rand_x;
	uv.y += noise_val * noise_weight * rand_y;

	vec3 colour = texture(image_texture, uv).rgb;
	colour += add_colour;

	vec2 center = vec2(0.5, 0.5);
	float dist = 1.0 - length(center - UV);

	// Fade
	float alpha = smoothstep(fade_radius, fade_radius + fade_softness, dist); 

	ALBEDO = vec3(colour);
	ALPHA = alpha;
}