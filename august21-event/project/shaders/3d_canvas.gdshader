shader_type spatial;

uniform sampler2D board_texture: filter_nearest;
uniform sampler2D palette_texture: filter_nearest;

//void vertex() {
//	
//}

void fragment() {
	float index = texture(board_texture, vec2(UV.x, 1.0 - UV.y)).r;
	vec4 tile = texelFetch(palette_texture, ivec2(int(index), 0), 0);
	ALBEDO.rgb = vec3(tile.r, 0.0, 0.0);
}
