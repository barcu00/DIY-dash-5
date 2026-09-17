"""Lossless conversion of production LVGL framebuffers, no UI drawing."""
import sys
from pathlib import Path
from PIL import Image
output = Path(sys.argv[1])
paths = list(output.glob('firmware-*.ppm'))
assert len(paths) == 11
for path in paths:
    with Image.open(path) as image:
        assert image.size == (800, 480)
        image.save(path.with_suffix('.png'))
