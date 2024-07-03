# This file makes use of py360convert to process the cubemap images into a godot-compatible panoramic sky
# py360convert *does* have a pip package, but it is outdated, and contains a bug for our particular usecase,
# so we use the submodule instead
import numpy as np
from PIL import Image
import py360convert.py360convert as py360convert
from os import path

top_img = np.array(Image.open(path.join("cubemap", "top.png")))
right_img = np.array(Image.open(path.join("cubemap", "right.png")))
back_img = np.array(Image.open(path.join("cubemap", "back.png")))
bottom_img = np.array(Image.open(path.join("cubemap", "bottom.png")))
left_img = np.array(Image.open(path.join("cubemap", "left.png")))
front_img = np.array(Image.open(path.join("cubemap", "front.png")))

cube_dict = {
    'F': front_img,
    'R': right_img,
    'B': back_img,
    'L': left_img,
    'U': top_img,
    'D': bottom_img
}

output_data = py360convert.c2e(cube_dict, 1920, 3840, cube_format="dict")
Image.fromarray(output_data.astype(np.uint8)).save("roof_sky.png")
print("Conversion completed. Equirectangular image saved as roof_sky.png")
