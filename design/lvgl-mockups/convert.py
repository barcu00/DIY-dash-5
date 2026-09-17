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
        if path.stem in {'lvgl-analog-d-outer-pointer', 'lvgl-layout-e-semicircle'}:
            # Inspect actual rendered digits, not nominal font line rectangles.
            # The right-side numeric text is white; labels/units are muted.
            pixels = image.convert('RGB')
            for row in range(6):
                tile_y = 16 + row * 68
                digit_rows = [y for y in range(tile_y + 1, tile_y + 54)
                              if any(min(pixels.getpixel((x, y))) > 210
                                     for x in range(600, 780))]
                assert digit_rows, (path.stem, row, 'missing numeric value')
                visible_center = (min(digit_rows) + max(digit_rows)) / 2
                assert abs(visible_center - (tile_y + 30.5)) <= 1.5, (
                    path.stem, row, 'digits are not vertically centered', visible_center)
        image.save(path.with_suffix('.png'))
