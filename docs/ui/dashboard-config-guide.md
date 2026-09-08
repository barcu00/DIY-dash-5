# Dashboard configuration guide

This firmware targets the Waveshare ESP32-S3-Touch-LCD-5 non-B at 800x480.
The bottom navigation contains DASH, TRACK, and SETTINGS; DIAG is not present.

## Tiles and layout

Hold a visible tile for about 600 ms to open its editor. Choose the parameter,
visibility, decimal places, and optional warning. SAVE TILE applies the draft in
RAM, closes the editor, and queues one persistent transaction. The application
loop writes it outside the LVGL callback. Cancel leaves the configuration
unchanged; a failed persistent write reports `SAVE ERROR` and remains retryable.

Hidden tiles can be reopened from the paged DASH and TRACK slot lists in the
LAYOUTS category. Each page contains no more than six slots. Hiding a tile
compacts only its own logical group toward the bottom:
left, center-wide, center-small, or right on DASH; left, center-wide, or right
on TRACK. Wide tile labels and values are centered.

## Warnings

Each tile independently supports above/below direction, native threshold,
hysteresis, and delay. A breached warning produces a large red WARNING modal
with the current value and configured limit. Acknowledging removes the modal,
but the related visible tile stays red until the value returns through the safe
hysteresis boundary. A hidden tile can still raise its warning. Invalid data
does not raise a warning.

## Settings categories

SETTINGS opens a fixed six-button home screen. DISPLAY, DATA & CAN, SHIFT
LIGHT, UNITS, LAYOUTS, and SYSTEM each open as a separate 800x480 screen. The
firmware creates only the active settings screen, so there is no long scrolling
list to lay out or redraw.

Controls change the working configuration in RAM without writing flash. A dirty
category queues one complete configuration snapshot when the user presses BACK,
DASH, TRACK, or otherwise leaves that category. Repeated edits are coalesced,
and an unchanged category performs no write. `SAVING`, `SAVED`, and `SAVE ERROR`
provide non-modal status; a failed write keeps the RAM values and can be retried
by leaving again.

## Available settings

- Brightness uses a software overlay from 20 to 100 percent; there is no off
  switch.
- Source is explicitly CAN or DEMO. CAN never automatically falls back to DEMO.
- CAN is receive-only. Available bitrates are 125, 250, 500, and 1000 kbit/s;
  timeout is adjustable from 100 to 5000 ms.
- The profile picker offers `none`, five source-pinned standalone ECU profiles,
  and separately marked experimental Link and PSA C2 profiles. Selecting `none`
  is the safe no-decoder state. A parameter not supplied by the active profile
  displays `---`.
- START, RED, FLASH, and MAX RPM use four sliders shared by DASH and TRACK and
  must satisfy `start < red < flash <= maximum`. Values snap to 100 RPM steps,
  span 0-10000 RPM, and changing one threshold automatically moves dependent
  thresholds. Existing stored values above 10000 RPM are normalized without
  discarding unrelated settings.
- FLASH ENABLED controls whether the complete strip alternates red/off at about
  4 Hz once valid RPM reaches FLASH RPM. Disabling it keeps normal progressive
  behavior through MAX RPM. The 12 physical segments always use four green,
  four yellow, and four red positions.
- Temperature, pressure, speed, and mixture units affect presentation only.
  Stored telemetry and warning comparisons remain in native units.
- RESET DASH and RESET TRACK restore only the selected tile layout. FACTORY
  RESET clears only the `diy_dash` application namespace and restores all
  defaults. Every reset requires explicit confirmation.

## Build and hardware validation

GitHub Actions runs native tests, compiles the exact Waveshare target, produces
a merged full-flash BIN and component binaries, and renders deterministic UI
review images. Those checks prove compilation and packaging, not electrical,
touch, CAN-bus, thermal, or long-duration behavior. Complete the physical
acceptance checklist in the README after flashing a test board.

## Screen gallery

| DISPLAY | DATA & CAN | UNITS |
| --- | --- | --- |
| ![DISPLAY settings](screenshots/ui-preview-settings-display.png) | ![DATA and CAN settings](screenshots/ui-preview-settings-can.png) | ![UNITS settings](screenshots/ui-preview-settings-units.png) |

| LAYOUTS | SYSTEM |
| --- | --- |
| ![LAYOUTS settings](screenshots/ui-preview-settings-layouts.png) | ![SYSTEM settings](screenshots/ui-preview-settings-system.png) |

| Tile editor | Warning modal |
| --- | --- |
| ![Tile editor](screenshots/ui-preview-tile-editor.png) | ![Warning modal](screenshots/ui-preview-warning.png) |
