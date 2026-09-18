# Actual firmware-view preview harness

Uses the production TileEngine, TileView, RpmScaleView, ShiftLightView and
navigation code with real LVGL 8.4. No mocked widgets and no Pillow UI drawing.
Runs only on GitHub Actions. PPM framebuffer pixels are converted losslessly to
PNG for viewing. The 512 KiB allocator capacity, RGB565 and font configuration
match firmware. CI exercises the embedded pool-allocation hook. This does not
measure embedded RAM or performance.

Scenes cover all six presets, compact and analog flag states, a warning tile,
6000-RPM scale, red flash phases, 10000-RPM digit fitting and unsupported numeric
captions. Assertions test flash colors, flag-caption separation, digit width and
unavailable-caption bounds. It does not render the full settings/editor screens,
simulate CAN hardware, or replace physical touch/smoothness testing.

All 25 layout scenes plus two Analog flash scenes run in partial-buffer and
full-frame double-buffer direct modes;
their PNGs must be pixel-identical. Modern flash-off is checked across the
entire semicircle, and its actual dirty-pixel budget is limited to 120000 pixels.
Strip connected components must be identical upright 18 × 28 pixel rectangles.
Thin Strip ticks must remain vertical. Analog flash checks the whole segmented
ring, its permanent white index and a 130000-pixel dirty-redraw budget.

The transition executable also compiles production `ui.cpp`, replacing only
the physical brightness output. It drives actual LVGL events to test DASH/TRACK
layout returns, tile-save visibility and Analog-center parameter editing before
the next scheduled update. Native repository tests verify center persistence.

Preview data assignments follow the actual saved/default banks; they may differ
from the decorative parameters used in the approved static prototypes.
