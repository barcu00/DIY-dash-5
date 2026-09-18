# RGB display during NVS saves: XIP test candidate

Candidate source: `6bd7432a918ec676edfcac62e74ef7a7c25a5c3e` on
`dashboard-dev`. This is not a public release or a confirmed hardware fix.

## Scope and rationale

The reported transient corruption occurs after settings/tile saves, but not
when leaving layout settings without changes. The preceding software checks
cover stale first frames, not physical RGB scanout during flash writes.

Espressif documents that RGB bounce buffers cannot operate with external-memory
cache disabled during NVS/OTA writes. PSRAM instruction fetch and read-only data
placement allow cache operation during flash writes. This candidate addresses
that risk without changing UI geometry, save sequencing or persistence format.
See the [ESP-IDF RGB LCD documentation](https://docs.espressif.com/projects/esp-idf/en/v5.3.2/esp32s3/api-reference/peripherals/lcd/rgb_lcd.html).

## Reproducible SDK configuration

`platformio.ini` retains Arduino 3.1.1 and selects the official
`esp32-3.1.1-h.zip` SDK libraries using `platform_packages`. No manual SDK edits
are needed after cloning. The board uses octal PSRAM (`qio_opi`). The SDK variant
also enables 64-byte data-cache lines and O2 library optimization; this is not
solely a one-option change. See the
[official SDK configuration and PlatformIO instructions](https://github.com/esp-arduino-libs/arduino-esp32-sdk).

`rgb_sdk_requirements.h` checks actual SDK configuration for:

- PSRAM XIP;
- PSRAM instruction fetch and read-only data;
- 64-byte ESP32-S3 data-cache lines.

Build flags do not fake these settings. A mismatched SDK fails compilation.
The USB startup log reports Arduino version and the checked configuration.

## GitHub verification

- [Expected RED with the previous SDK](https://github.com/barcu00/DIY-dash-5/actions/runs/35334536763):
  ESP32-S3 compilation failed all three configuration requirements.
- [Candidate firmware workflow](https://github.com/barcu00/DIY-dash-5/actions/runs/35334774197):
  198 native tests and 24 Python packaging/UI contract tests passed. Target
  compilation, SDK guards and full-image packaging succeeded.
- [Production LVGL workflow](https://github.com/barcu00/DIY-dash-5/actions/runs/35334774231):
  first-visible-frame checks for DASH/TRACK layout changes and tile save,
  Analog center editing, and production framebuffer comparisons passed.
- Read-only review found no Critical/Important issue in the SDK change.

## Full firmware image

The successful firmware workflow's `DIY-Dash-firmware` artifact includes
`DIY-Dash-ESP32-S3-Touch-LCD-5-full.bin`, verified at 16777216 bytes.
Flash this full image at `0x0`; it overwrites saved NVS settings.

SHA-256: `EC426C2D7710C33EC3ED3D7ECFAFAFD61E6B4BF063EE4171AED93962AB2C4FBF`.

Static RAM: 49704 / 327680 bytes (15.2%); application flash:
905222 / 6553600 bytes (13.8%). These do not measure dynamic memory or PSRAM.

## Physical validation checklist

On Waveshare ESP32-S3-Touch-LCD-5 non-B, capture USB serial at 115200 baud:

1. Confirm successful startup, PSRAM availability and no allocation/VSYNC errors.
2. Change a layout and return directly to DASH, then repeat for TRACK.
3. Save tile parameter, visibility and warning changes repeatedly.
4. Repeat saves while demo RPM and shift-light flashing are active.
5. Check free PSRAM/heap diagnostics and longer-running display stability.

XIP adds instruction/read-only-data occupancy in PSRAM. Static build RAM is not
runtime free RAM. Two RGB framebuffers remain 1500 KiB and the LVGL pool remains
512 KiB. Actual remaining memory must be measured on the board.

XIP does not eliminate every possible source of RGB starvation or PSRAM
bandwidth contention. A successful host test/build cannot prove physical
save-time artefacts are gone. Full 16 MB image flashing at `0x0` overwrites NVS
settings; configure warnings/layouts again before testing saves.
