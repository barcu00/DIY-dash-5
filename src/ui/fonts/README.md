# Numeric display fonts

The approved dashboard style uses `race_digits_48/64/96/140`, generated from
Google Fonts' Rajdhani-Bold.ttf using lv_font_conv 1.5.3. License:
`Rajdhani-OFL.txt` (SIL OFL 1.1). The size names are the converter input sizes;
actual line heights are smaller because these subsets contain only numeric
glyphs. Montserrat 24 is the fallback for unavailable values and text.
Regenerate with the command below, replacing font, size and name with
`Rajdhani-Bold.ttf`, `48/64/96/140`, and `race_digits_<size>`.
Generated sources are committed; no converter is needed for compilation.

The older `dash_numeric_64/80` sources below remain available but are no longer
used by the five dashboard presets. Link-time garbage collection removes unused
fonts from the firmware image.

Generated from `scripts/built_in_font/Montserrat-Medium.ttf` in LVGL v8.4.0
using the official `lvgl/lv_font_conv` converter, npm release 1.5.3.
Montserrat is distributed under SIL Open Font License 1.1 (see OFL.txt).
Only space, plus, minus, period, and digits are included; the normal 24 px
Montserrat font is the fallback. Output is uncompressed 4-bpp antialiased
bitmap data in flash, not runtime RAM or an extra build dependency.

```sh
lv_font_conv --size 64 --bpp 4 --format lvgl \
  --font Montserrat-Medium.ttf -r 0x20,0x2B-0x2E,0x30-0x39 \
  --no-compress --no-prefilter --lv-fallback lv_font_montserrat_24 \
  --lv-font-name dash_numeric_64 -o dash_numeric_64.c
```

Use size/name 80 for the gear font. Generated C sources are committed so
PlatformIO users do not need Node.js or the converter to compile firmware.
