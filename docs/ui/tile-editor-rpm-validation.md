# Tile editor and shared RPM candidate

Source: [`f176b7fc3816e53c25e250c832f7c307b58c5049`](https://github.com/barcu00/DIY-dash-5/commit/f176b7fc3816e53c25e250c832f7c307b58c5049), branch `dashboard-dev`.
This is a development candidate, not a new public release.

## Verified on GitHub

- [Firmware workflow](https://github.com/barcu00/DIY-dash-5/actions/runs/35399317213): 207 native C++ tests, 23 Python tests, Waveshare target compilation and full-image packaging passed.
- [Production LVGL workflow](https://github.com/barcu00/DIY-dash-5/actions/runs/35399317115): real UI navigation, keypad apply/cancel, direction changes, parameter defaults, save failure/retry, destination first frames and Analog center editing passed.
- Seven editor screenshots compare pixel-identically between partial and direct rendering. The 27 production scenes also compare both modes; incremental/full redraw checks cover the main gauges and flash phases.
- kPa/PSI warnings, Fahrenheit temperature limits and a 250-ms delay retain exact native values when untouched. Regression tests failed before their fixes and passed afterward.
- Read-only code review found no remaining Important or Critical issue after fixes. Hardware acceptance is still required.

RED evidence includes missing-model tests in runs `35397782885` and `35397847518`, editor/default-decimal regressions in `35398832309` / `35398832276`, and untouched-unit/delay regressions in `35399183755`. Tests and builds were run on GitHub, not locally.

## Binary and memory

Download `DIY-Dash-firmware` from the successful firmware run above and extract
`DIY-Dash-ESP32-S3-Touch-LCD-5-full.bin`.

- Full image: **16,777,216 bytes**, flash offset **`0x0`**.
- SHA-256: `59C232D5BFC0B158F4D895594FD3018CD75824954D65B1F969824DF20AF52A2A`.
- Static internal RAM: **50,504 / 327,680 bytes (15.4%)**.
- Application flash: **970,578 / 6,553,600 bytes (14.8%)**.

Static RAM excludes runtime heaps, task stacks, panel buffers and PSRAM allocations;
it is not a measurement of free memory during operation. The existing two full RGB
buffers and 512-KiB PSRAM LVGL pool remain unchanged, as do Arduino 3.1.1-h XIP
guards and VSYNC-controlled swaps.

**Flashing the full image overwrites NVS and resets saved settings.** Target only
the Waveshare ESP32-S3-Touch-LCD-5 **non-B**, not a similarly named board.

## Physical-board acceptance

1. Boot, then navigate all six layouts on DASH and TRACK.
2. Long-press a tile; inspect DATA, WARNING and temperature-only tabs.
3. Apply and cancel keypad edits; select numeric and flag parameters and hide/restore a tile.
4. Repeat SAVE TILE and settings exits; check for artefacts, stale first frames, freezes and restart persistence.
5. Check PSI/kPa/Fahrenheit thresholds and reset boundaries, including negative temperatures.
6. TEST WARNING must leave real alarms unchanged; its buzzer pulse must respect WARNING SOUND. Startup chirp remains independent.
7. Change shared RPM scale and thresholds, then inspect full-ring/strip flashing at 4 Hz, including Modern Motorsport.
8. Run at least 15 minutes and verify CAN reception/stale-data behavior.

Host renders cannot establish physical display timing, electrical behavior or
smoothness. Prior XIP stability was user-confirmed; this new editor still needs
its own board test. Current captures are linked in the [configuration guide](dashboard-config-guide.md).
