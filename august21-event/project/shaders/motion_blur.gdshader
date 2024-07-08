/**************************************************************************/
/*  motion_blur.gdshader                                                  */
/**************************************************************************/
/*   Modified version of motion blur shader created by Mantequilla, see   */
/*           (https://godotengine.org/asset-library/asset/1499)           */
/*        (https://github.com/davisbrandon02/motion-blur-godot-4.0)       */
/*     (https://godotengine.org/asset-library/asset?user=Mantequilla)     */
/**************************************************************************/
/*                   Released under the MIT license:                      */
/* Copyright (c) 2022 Brandon Davis                                       */
/*                                                                        */
/* Permission is hereby granted, free of charge, to any person obtaining  */
/* a copy of this software and associated documentation files (the        */
/* "Software"), to deal in the Software without restriction, including    */
/* without limitation the rights to use, copy, modify, merge, publish,    */
/* distribute, sublicense, and/or sell copies of the Software, and to     */
/* permit persons to whom the Software is furnished to do so, subject to  */
/* the following conditions:                                              */
/*                                                                        */
/* The above copyright notice and this permission notice shall be         */
/* included in all copies or substantial portions of the Software.        */
/*                                                                        */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,        */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF     */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. */
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY   */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,   */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE      */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                 */
/**************************************************************************/
shader_type spatial;
render_mode depth_test_disabled, depth_draw_never, unshaded, cull_disabled;

uniform vec3 linear_velocity; 
uniform vec3 angular_velocity; //rads
uniform int iteration_count : hint_range(2, 50);
uniform float intensity : hint_range(0, 1);
uniform float start_radius;
uniform sampler2D DEPTH_TEXTURE : hint_depth_texture, filter_linear_mipmap;
uniform sampler2D SCREEN_TEXTURE : hint_screen_texture, filter_linear_mipmap;

void fragment()
{ 
	float depth = texture(DEPTH_TEXTURE, SCREEN_UV).r;
	
	//Turn the current pixel from ndc to world coordinates
	vec3 pixel_pos_ndc = vec3(SCREEN_UV*2.0-1.0, depth*2.0-1.0); 
    vec4 pixel_pos_clip = INV_PROJECTION_MATRIX * vec4(pixel_pos_ndc,1.0);
    vec3 pixel_pos_cam = pixel_pos_clip.xyz / pixel_pos_clip.w;
	vec3 pixel_pos_world = (inverse(INV_VIEW_MATRIX) * vec4(pixel_pos_cam, 1.0)).xyz;
	
	//Calculate total velocity which combines linear velocity and angular velocity
	vec3 cam_pos = inverse(INV_VIEW_MATRIX)[3].xyz; //Correct
	vec3 r = pixel_pos_world - cam_pos;
	vec3 total_velocity = linear_velocity + cross(angular_velocity, r);
	
	//Offset the world pos by the total velocity, then project back to ndc coordinates
	vec3 pixel_prevpos_world = pixel_pos_world - total_velocity;
	vec3 pixel_prevpos_cam =  ((INV_VIEW_MATRIX) * vec4(pixel_prevpos_world, 1.0)).xyz;
	vec4 pixel_prevpos_clip =  PROJECTION_MATRIX * vec4(pixel_prevpos_cam, 1.0);
	vec3 pixel_prevpos_ndc = pixel_prevpos_clip.xyz / pixel_prevpos_clip.w;
	
	//Calculate how much the pixel moved in ndc space
	vec2 pixel_diff_ndc = pixel_prevpos_ndc.xy - pixel_pos_ndc.xy; 
	
	vec3 col = vec3(0.0);
	float counter = 0.0;
	if(length(r) >= start_radius){
		for (int i = 0; i < iteration_count; i++)
		{
			vec2 offset = pixel_diff_ndc * (float(i) / float(iteration_count) - 0.5) * intensity; 
			col += textureLod(SCREEN_TEXTURE, SCREEN_UV + offset,0.0).rgb;
			counter++;
		}
		ALBEDO = col / counter;
	}
	else
		ALBEDO = textureLod(SCREEN_TEXTURE, SCREEN_UV, 0.0).rgb;
}
