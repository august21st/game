/*
 * The contents of this file are based off the original work [The simplest outline shader (via material)]
 * (https://godotshaders.com/shader/the-simplest-outline-shader-via-material/),
 * by [rpaciorek](https://godotshaders.com/author/rpaciorek/), released freely into the public domain under the
 * [Creative Commons Zero v1.0 Universal license](https://creativecommons.org/publicdomain/zero/1.0/).
 *
 * ### Attribution:
 * - Original source by [rpaciorek](https://godotshaders.com/author/rpaciorek/).
 * - Modifications by Zekiah-A & contributors.
 *
 * ### Disclaimer:
 * This adaptation is not endorsed by the original licensor and is provided without warranties,
 * this adaptation is also public domain, and is not licensed under the project's primary license
 * ([Creative Commons Zero v1.0 Universal license](https://creativecommons.org/publicdomain/zero/1.0/deed.en)).
 */
shader_type spatial;
render_mode blend_mix, cull_front, unshaded;

uniform vec4 colour : source_color = vec4(1.0, 0.0, 0.0, 1.0);
uniform float size : hint_range(1.0, 1.5, 0.01) = 1.1;

void vertex() {
	VERTEX *= size;
}

void fragment() {
	ALBEDO = colour.rgb;
	ALPHA = colour.a;
}
