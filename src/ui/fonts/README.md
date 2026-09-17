# Numeric display fonts

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
