# Numeric display fonts

The approved dashboard style uses `race_digits_48/64/96/140`, generated from
Google Fonts' Rajdhani-Bold.ttf using lv_font_conv 1.5.3. License:
`Rajdhani-OFL.txt` (SIL OFL 1.1). The size names are the converter input sizes;
actual line heights are smaller because these subsets contain only numeric
glyphs. Montserrat 24 is the fallback for unavailable values and text.
The race digits also contain `:` for lap times (`1:42.38`). They are
regenerated from `rajdhani-latin-700-normal.woff` (npm `@fontsource/rajdhani`
5.3.0, same Rajdhani Bold outlines) with:

```sh
lv_font_conv --size 96 --bpp 4 --format lvgl \
  --font rajdhani-latin-700-normal.woff -r 0x20,0x2B-0x2E,0x30-0x3A \
  --no-compress --no-prefilter --lv-fallback lv_font_montserrat_24 \
  --lv-font-name race_digits_96 -o race_digits_96.c
```

Use sizes/names 48, 64 and 140 for the other race digit fonts.
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


## Analog layout fonts

`dash_italic_*` (Barlow Condensed 600/700 italic, numbers; sizes 22 and 26
also contain A-Z for the RPM readout) and `dash_label_*` (Barlow 600/500,
printable ASCII plus ° and ×) style ANALOG CLASSIC and ANALOG TWIN. Both
families are SIL OFL 1.1 (Google Fonts, npm `@fontsource/barlow-condensed`
and `@fontsource/barlow` 5.3.0). Generated with lv_font_conv 1.5.3, e.g.:

```sh
lv_font_conv --size 48 --bpp 4 --format lvgl \
  --font barlow-condensed-latin-600-italic.woff -r 0x20,0x2B-0x3A \
  --no-compress --no-prefilter --lv-fallback lv_font_montserrat_24 \
  --lv-font-name dash_italic_48 -o dash_italic_48.c
lv_font_conv --size 12 --bpp 4 --format lvgl \
  --font barlow-latin-600-normal.woff -r 0x20-0x7E,0xB0,0xD7 \
  --no-compress --no-prefilter --lv-fallback lv_font_montserrat_12 \
  --lv-font-name dash_label_12 -o dash_label_12.c
```
