"""Lossless format conversion only; all UI pixels are rendered by LVGL."""
import sys
from pathlib import Path
from PIL import Image

output = Path(sys.argv[1])
expected = {'lvgl-analog-style', 'lvgl-side-gear', 'lvgl-strip-style',
            'lvgl-classic-dash', 'lvgl-classic-track',
            'lvgl-analog-motorsport-v3', 'lvgl-strip-circular-v3',
            'lvgl-side-gear-v3', 'lvgl-classic-dash-v3', 'lvgl-classic-track-v3',
            'lvgl-analog-7500-v3', 'lvgl-strip-7500-v3',
            'lvgl-analog-d-outer-pointer', 'lvgl-layout-e-semicircle',
            'lvgl-layout-e-7500', 'lvgl-layout-e-idle', 'lvgl-layout-e-redline'}
assert {path.stem for path in output.glob('*.ppm')} == expected
for path in output.glob('*.ppm'):
    with Image.open(path) as image:
        assert image.size == (800, 480)
        image.save(path.with_suffix('.png'))
