// Modified shader from https://godotshaders.com/shader/lava-shader/ for 3D
// creadits to https://godotshaders.com/author/a-python-script/ for making
// the original.
shader_type spatial;

uniform float timeScaleFactor = .04;

uniform sampler2D noise_texture: repeat_enable, filter_linear;

void light()
{
}

float local_time()
{
	return TIME * timeScaleFactor;
}

mat2 makem2(float theta)
{
	float c = cos(theta);
	float s = sin(theta);
	return mat2(vec2(c, -s), vec2(s, c));
}

float noise(vec2 x)
{
	return texture(noise_texture, x * .01).x;
}

vec2 gradn(vec2 p)
{
	float ep = .09;
	float gradx = noise(vec2(p.x + ep, p.y)) - noise(vec2(p.x - ep, p.y));
	float grady = noise(vec2(p.x, p.y + ep)) - noise(vec2(p.x, p.y - ep));
	return vec2(gradx, grady);
}

float flow(vec2 p)
{
	float z = 2.;
	float rz = 0.;
	vec2 bp = p;
	
	for(float i = 1.; i < 7.; ++i) {
		p += local_time() * .6;
		bp += local_time() * 1.9;
		vec2 gr = gradn(i * p * .34 + local_time() * 1.);
		gr *= makem2(local_time() * 6. - (.05 * p.x + 0.03 * p.y) * 40.);
		p += gr * .5;
		rz += (sin(noise(p) * 7.) *.5 + .5) / z;
		p = mix(bp, p, .77);
		z *= 1.4;
		p *= 2.;
		bp *= 1.9;
	}
	return rz;
}

void fragment()
{
	vec2 p = VERTEX.xy;
	p *= VERTEX.z;
	float rz = flow(p);
	vec3 colour = vec3(.2, .07, .01) / rz;
	colour = pow(colour, vec3(1.4));
	METALLIC = 0.0;
	ROUGHNESS = 0.0;
	EMISSION = colour;
	ALBEDO = colour;
}