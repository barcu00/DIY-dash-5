"""Lossless format conversion only; all UI pixels are rendered by LVGL."""
import sys
from pathlib import Path
from PIL import Image

output = Path(sys.argv[1])
expected = {'lvgl-analog-style', 'lvgl-side-gear', 'lvgl-strip-style',
            'lvgl-classic-dash', 'lvgl-classic-track'}
assert {path.stem for path in output.glob('*.ppm')} == expected
for path in output.glob('*.ppm'):
    with Image.open(path) as image:
        assert image.size == (800, 480)
        image.save(path.with_suffix('.png'))
