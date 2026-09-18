# Display stability candidate

Firmware source: `8a56d779fd1ee69d118786cfe4eeb83d3c707e7e`, branch `dashboard-dev`.
This is a test candidate, not a published release. Tests and compilation ran
only on GitHub.

## Changes

- Two complete RGB565 framebuffers (1500 KiB total), with panel swaps at VSYNC.
  LVGL waits for ownership transfer before drawing into the previous buffer.
- LVGL's 512 KiB pool is allocated in PSRAM before initialization. Allocation
  failures and refresh timeouts are reported rather than silently hanging.
- Modern Motorsport flashes one complete semicircle at the existing 4 Hz;
  flash phases do not repaint the whole tachometer or recolor the RPM text.
- Strip segments are identical upright 18 × 28 pixel rectangles, not rotated
  polygons. The arc is formed by their vertical positions.
- USB serial diagnostics every five seconds report rendering time, LVGL free
  memory, fragmentation, heap, PSRAM and stack headroom. Read at 115200 baud.

## Verification

- [Firmware workflow](https://github.com/barcu00/DIY-dash-5/actions/runs/35324913081):
  197 native tests and 24 Python packaging/UI contract tests passed; target
  compilation and full-image packaging passed.
- [Production LVGL workflow](https://github.com/barcu00/DIY-dash-5/actions/runs/35324913078):
  25 scenes in each of two buffer modes passed, with pixel-identical output.
  Incremental/full redraw comparisons, whole-arc flash colors, a 120000-pixel
  dirty-redraw budget and constant Strip block geometry passed.
- Regression tests failed before the corresponding changes: the Modern flash
  dirty-pixel budget and the complete-frame buffer capacity check.
- Read-only review of framebuffer ownership and the allocation hook completed;
  its three concrete findings were corrected before these successful runs.

Build reports static RAM 48072 / 327680 bytes (14.7%) and application flash
835196 / 6553600 bytes (12.7%). Static RAM excludes dynamic allocations and PSRAM;
it is not the total runtime memory usage.

## Flash image

Download `DIY-Dash-firmware` from the firmware workflow above. Use only
`DIY-Dash-ESP32-S3-Touch-LCD-5-full.bin` for a complete flash at `0x0` on the
Waveshare ESP32-S3-Touch-LCD-5 non-B. Expected size: 16777216 bytes.

SHA-256: `DCC9BE66B07322041480488FF227E9037687558A15A3BFDDF7A07211205B2E8A`.

**Full-image flashing overwrites NVS settings. Record your settings first.**

The screenshots are actual production-view LVGL captures, not artistic mockups.
Host rendering cannot prove physical display timing or stability. Test DEMO,
flash phases, repeated DASH/TRACK switching, settings, touch and brightness on
the board. If stalls persist, retain the USB serial diagnostic/error output.
