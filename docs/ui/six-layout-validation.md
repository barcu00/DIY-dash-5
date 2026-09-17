# Six-layout firmware validation

Production source: `7e65aac5b817b371db9bff2686e8b239e33c18a5`, branch `dashboard-dev`.

All compilation, tests and LVGL framebuffer rendering ran on GitHub, not locally.

- [Firmware workflow](https://github.com/barcu00/DIY-dash-5/actions/runs/35283077896): 197 native tests and 24 Python packaging/UI contract tests passed; ESP32-S3 compilation and binary packaging passed.
- [Production framebuffer workflow](https://github.com/barcu00/DIY-dash-5/actions/runs/35283077868): 25 real LVGL 8.4 scenes passed, including optical numeric centering, cyan rails, flags, unavailable captions, flash phases, 7500 RPM scales and hidden/warning tiles.
- Incremental versus full framebuffer comparisons passed for Analog D, Side Gear, Strip and Modern Motorsport E. This includes small RPM changes, threshold crossings, flashing and signal invalidation.
- Test-only commit `05e8c55` reproduced stale incremental pixels before the complete-sector invalidation fix in `7e65aac`.
- Read-only review identified the Side Gear redraw defect and redundant CompactRow text measurements; both were addressed before the successful runs.

Build reports static RAM 146336 / 327680 bytes (44.7%) and application flash 818496 / 6553600 bytes (12.5%). RAM figures do not include dynamic heap, display buffers or PSRAM usage.

The full image is `DIY-Dash-ESP32-S3-Touch-LCD-5-full.bin`, 16777216 bytes, for Waveshare ESP32-S3-Touch-LCD-5 non-B. Flash at `0x0`. Obtain it from the firmware workflow artifact; it is not a new public release.

Full-image SHA-256: `AD6CFBAFDA530114907E7A3675A003F17F651164E9419A1C3ACABB0818B12B7A`.

**Full-image flashing overwrites the complete flash, including NVS settings. Back up existing settings first.** Schema v8 migrates retained v6/v7 NVS settings when only the application is updated; migration cannot restore NVS erased by a full image.

The accompanying `firmware-*.png` files are actual production-view framebuffer captures, not artistic mockups. Automated equality checks do not prove on-device refresh rate, touch latency or electrical stability. Physical board and vehicle validation remain required.
