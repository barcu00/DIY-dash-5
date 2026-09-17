# Actual firmware-view preview harness

Uses the production TileEngine, TileView, RpmScaleView, ShiftLightView and
navigation code with real LVGL 8.4. No mocked widgets and no Pillow UI drawing.
Runs only on GitHub Actions. PPM framebuffer pixels are converted losslessly to
PNG for viewing. Desktop allocator capacity is enlarged; RGB565 and font
configuration match firmware. This does not measure embedded RAM or performance.

Scenes cover all five presets, compact and analog flag states, a warning tile,
6000-RPM scale, red flash phases, 10000-RPM digit fitting and unsupported numeric
captions. Assertions test flash colors, flag-caption separation, digit width and
unavailable-caption bounds. It does not render the full settings/editor screens,
simulate CAN hardware, or replace physical touch/smoothness testing.

Preview data assignments follow the actual saved/default banks; they may differ
from the decorative parameters used in the approved static prototypes.
