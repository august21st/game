# The scale of this map is about 1:10, intended to be a skybox for the roof level
# The sky can be converted to a panorama with the following:
# https://danilw.github.io/GLSL-howto/cubemap_to_panorama_js/cubemap_to_panorama.html
# This script is intended to automate the rendering process
import bpy
import math

print("Started rendering...")

camera_rotations = {
    "front": (90, 0, 0),
    "right": (90, 0, -90),
    "back": (90, 0, 180),
    "left": (90, 0, 90),
    "top": (180, 0, 0),
    "bottom": (0, 0, 0)
}

output_dir = "./cubemap/"

if not output_dir.endswith('/'):
    output_dir += '/'

def render_image(direction, rotation):
    camera = bpy.context.scene.camera
    camera.rotation_euler = (math.radians(rotation[0]), math.radians(rotation[1]), math.radians(rotation[2]))
    bpy.context.scene.render.filepath = f"{output_dir}{direction}.png"
    bpy.ops.render.render(write_still=True)

for direction, rotation in camera_rotations.items():
    render_image(direction, rotation)

print("Rendering complete! Cubemap images output to ./cubemap/ directory.")