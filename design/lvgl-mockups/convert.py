"""Lossless format conversion only; all UI pixels are rendered by LVGL."""
import sys
from pathlib import Path
from PIL import Image

for path in Path(sys.argv[1]).glob('*.ppm'):
    with Image.open(path) as image:
        assert image.size == (800, 480)
        image.save(path.with_suffix('.png'))
