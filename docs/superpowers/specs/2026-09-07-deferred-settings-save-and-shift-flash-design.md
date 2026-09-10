# Deferred Settings Save and Shift Flash Design

## Goal

Eliminate display artifacts caused by synchronous NVS writes while controls are being used, and extend the DASH and TRACK shift-light strip with slider-based RPM thresholds plus an optional full-strip red flash.

## Root Cause

The current SETTINGS callbacks call `ConfigRepository::saveCandidate()` directly. That operation writes the complete configuration blob through `Preferences::putBytes()` while LVGL is processing an input event. Repeated brightness or RPM changes therefore combine flash/cache stalls with active RGB-panel rendering, which can produce tearing, stale regions, and displaced screen content. The tile editor does not reproduce the problem while values are being changed because it keeps edits in memory until the editor is saved.

The fix must remove persistent storage writes from LVGL callbacks rather than trying to hide the resulting redraw damage.

## Settings Editing Model

SETTINGS uses a working configuration in RAM:

- Controls update the working values immediately.
- Brightness preview remains immediate so the user can see the selected level.
- A category is marked dirty only when its effective value changes.
- No NVS write occurs while a slider, dropdown, switch, or step control is manipulated.
- Leaving a dirty category through BACK, DASH, TRACK, or another category queues one complete configuration snapshot for persistence.
- Leaving an unchanged category queues nothing.
- Repeated changes before leaving are coalesced into that single snapshot.

Navigation completes first. The application loop then consumes the queued snapshot and calls the repository outside the LVGL event callback. Runtime reconfiguration for CAN-related changes occurs after the queued configuration has been accepted. This keeps input and screen creation independent of flash latency.

All configuration commits initiated by SETTINGS, including layout-editor saves and confirmed resets, use the same application-loop queue so no SETTINGS callback writes NVS directly.

## Save State and Failure Handling

Only one pending configuration snapshot is required. A newer exit replaces an older snapshot that has not yet been written, ensuring that the most recent complete configuration wins.

The visible destination screen receives non-modal status feedback:

- `SAVING` while a queued write is pending;
- `SAVED` after a successful write;
- `SAVE ERROR` after a failed write.

After completion, LVGL invalidates the active screen once. A failed write does not undo the working RAM values. The configuration remains dirty so a later exit can retry. If persistence succeeds, the saved snapshot becomes the new clean baseline.

## Shift-Light Configuration

The SHIFT LIGHT category contains four horizontal sliders with numeric RPM labels:

1. `START RPM`
2. `RED RPM`
3. `FLASH RPM`
4. `MAX RPM`

It also contains a `FLASH ENABLED` switch. Slider changes update their labels and the working configuration without writing NVS.

The model enforces:

`START RPM < RED RPM < FLASH RPM <= MAX RPM`

The supported RPM range and step remain bounded by the firmware. When one slider would violate the invariant, dependent thresholds are moved to the nearest valid step and every affected slider and label is refreshed together. This prevents an invalid intermediate configuration from reaching persistence or the renderer.

The default flash threshold is below or equal to the default maximum and the feature is enabled by default. Existing installations are migrated to the new schema without discarding tile layouts or other saved settings: the legacy maximum supplies a safe initial flash threshold, and the remaining fields are copied unchanged.

## Flash Rendering

The same behavior applies to the shift-light strips on DASH and TRACK:

- Below `FLASH RPM`, the existing progressive green/yellow/red segment behavior remains unchanged.
- At or above `FLASH RPM`, when flashing is enabled, all segments alternate between bright red and off.
- The flash rate is approximately 4 Hz, using a 125 ms half-period.
- Timing uses the existing monotonic application time; it never blocks the main loop.
- When flashing is disabled, the strip continues to show its normal red-zone progression up to `MAX RPM`.
- Missing or invalid RPM data never activates flashing.

Only segment styles whose state has changed are updated, avoiding unnecessary full-strip redraws. DASH and TRACK share the same flash phase and configuration.

## Persistence Schema

`ShiftLightConfig` gains `flash_rpm` and `flash_enabled`, requiring an `AppConfig` schema-version increment. The repository recognizes the immediately previous schema and converts it to the new structure before validation and saving. Unknown or corrupt schemas continue to fall back to defaults. This preserves existing user tile, CAN, unit, and brightness configuration during the firmware upgrade.

## Test Strategy

All compilation, unit tests, UI-preview generation, and firmware packaging run only in GitHub Actions.

The RED/GREEN test sequence covers:

- settings changes do not call persistence during control events;
- one dirty snapshot is queued on every supported exit path;
- unchanged exits do not queue a write;
- multiple edits are coalesced into the latest snapshot;
- repository writes occur from application-loop servicing rather than an LVGL callback;
- success establishes a clean baseline and failure remains retryable;
- all four RPM thresholds enforce `START < RED < FLASH <= MAX`;
- flash enable/disable persistence and legacy-schema migration;
- normal shift-light progression below the flash threshold;
- synchronized full-red/off phases at and above the threshold;
- no flashing for invalid RPM data;
- the 800x480 SHIFT LIGHT screen fits without scrolling and exposes four sliders plus the switch.

GitHub Actions must finish successfully and produce UI previews plus a flashable full-board BIN. The artifact's workflow URL, size, and SHA-256 checksum are reported after download.

## Acceptance Criteria

- Repeated brightness and shift-light adjustments perform no NVS writes and do not corrupt the displayed layout.
- A dirty category is saved once when the user leaves it; unchanged categories are not saved.
- No SETTINGS LVGL callback performs a flash write.
- SHIFT LIGHT uses four sliders and a flash-enable control.
- DASH and TRACK flash the complete strip red above the independent threshold when enabled.
- Existing stored settings survive migration from the previous schema.
- GitHub Actions passes and provides the test firmware BIN and updated UI previews.
