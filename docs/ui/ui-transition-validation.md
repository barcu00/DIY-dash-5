# UI transitions and configurable Analog center

Test candidate source: `19a678a1b88c5752691d386b741c9df1dad2c1a6`,
branch `dashboard-dev`. This candidate is not a public release.

## Changes

- Prepare retained DASH/TRACK layouts and cached telemetry before loading the
  destination screen. Successful tile saves use the same preparation path.
- Strip's fine tick marks are vertical, matching its upright rectangular LEDs.
- Frames and tile rails use neutral grey `#707780`; navigation cyan and semantic
  warning/temperature/RPM colors are retained.
- Analog's 54 ring segments flash red/dim at 4 Hz above the configured flash
  threshold. Its moving rectangular index remains white.
- Analog's center is normal editable tile slot 6. Long press opens the existing
  editor for parameter selection, visibility, warnings, decimals and supported
  temperature/flag options. Each page retains independent settings. The RPM
  ring and index do not change source when the center parameter changes.
- Existing schema-v8 storage already contains 14 slots per alternate bank.
  Previously unused Analog slot 6 defaults to visible RPM; no storage-layout
  change or migration is needed. Full-image flashing still erases stored data.

## GitHub verification

- [Native tests and firmware build](https://github.com/barcu00/DIY-dash-5/actions/runs/35330713989).
  Native tests: 198 passed. Python packaging/UI contract tests: 24 passed.
- [Real LVGL production tests](https://github.com/barcu00/DIY-dash-5/actions/runs/35330714000):
  four first-frame/editor integration scenarios and 27 scenes in each of two
  buffer modes passed. Partial/direct PNGs are pixel-identical. Incremental/full
  rendering comparisons passed, including ring flash phases and invalid data.
- Red regression runs demonstrated the stale layout, delayed saved visibility,
  tilted Strip ticks, cyan rails and missing Analog-center editing/ring flashing
  before their fixes.
- Analog flash has a 130000-pixel dirty-render budget; Modern retains 120000.
  These are host rendering budgets, not on-device refresh-rate measurements.
- Read-only review found no concrete Critical/Important defect. Additional tests
  cover TRACK return, center editing/persistence and white-index flash colors.

## Firmware image

The firmware workflow above completed target compilation and full-image packaging.
Its `DIY-Dash-firmware` artifact contains
`DIY-Dash-ESP32-S3-Touch-LCD-5-full.bin`, 16777216 bytes. Flash at `0x0` on
Waveshare ESP32-S3-Touch-LCD-5 non-B. **This overwrites NVS settings.**

SHA-256: `8F1660C27F7F6DBC1DD82EC6ABCFB83D4B952CADA6C9F46913C905F829754998`.

Static RAM: 49320 / 327680 bytes (15.1%); application flash:
836000 / 6553600 bytes (12.8%). These figures exclude dynamic memory and PSRAM.
RGB framebuffers remain 1500 KiB and the LVGL PSRAM pool remains 512 KiB.

## Physical validation remains necessary

Test switching layouts and saving tile parameters/visibility on the actual board.
The first-frame software defects are covered above; this does not prove all
physical RGB disturbances during a flash write are eliminated. Espressif notes
that RGB bounce-buffer operation can be disrupted while external-memory cache
is disabled for NVS writes. See the
[ESP-IDF RGB LCD documentation](https://docs.espressif.com/projects/esp-idf/en/v5.2/esp32s3/api-reference/peripherals/lcd.html).
If save-time artifacts remain, capture USB serial output at 115200 baud and
note whether they occur during `SAVING` or on return to the data page.
