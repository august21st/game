shader_type canvas_item;

uniform float opacity: hint_range(0.0, 1.0, 0.1) = 1.0;

void fragment()
{
	vec2 centre = -1.0 + 2.0 * UV;
	float dist = length(centre);
	vec2 ripple_uv = UV + (centre / dist) * cos(dist * 12.0 - TIME * 4.0) * 0.02;
	vec4 colour = texture(TEXTURE, ripple_uv);
	COLOR = vec4(colour.rgb, colour.a * opacity);
}
