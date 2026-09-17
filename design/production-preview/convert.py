"""Lossless conversion of production LVGL framebuffers, no UI drawing."""
import sys
from pathlib import Path
from PIL import Image
output = Path(sys.argv[1])
paths = list(output.glob('firmware-*.ppm'))
assert len(paths) == 25
for path in paths:
    with Image.open(path) as image:
        assert image.size == (800, 480)
        if path.stem in {'firmware-analog-style','firmware-modern-motorsport'}:
            pixels=image.convert('RGB')
            for row in range(6):
                top=16+row*68
                ys=[y for y in range(top+1,top+54)
                    if any(min(pixels.getpixel((x,y)))>210 for x in range(600,780))]
                assert ys, ('missing digits',row)
                assert abs((min(ys)+max(ys))/2-(top+30.5))<=1.5, ('off-center digits',row,ys)
        image.save(path.with_suffix('.png'))
