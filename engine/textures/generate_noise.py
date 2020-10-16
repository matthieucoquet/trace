import numpy as np
from PIL import Image

# See https://www.shadertoy.com/view/4sfGzS

side = 512

array = np.zeros([side, side, 3], dtype=np.uint8)
array[:, :, 0] = np.random.randint(0, 256, size=(size, size))
array[:, :, 1] = np.roll(array[:, :, 0], (17, 37), axis=(0, 1))

image = Image.fromarray(array)
image.save('lut_noise.png')