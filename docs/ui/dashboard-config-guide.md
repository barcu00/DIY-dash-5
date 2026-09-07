# Dashboard configuration guide

This firmware targets the Waveshare ESP32-S3-Touch-LCD-5 non-B at 800x480.
The bottom navigation contains DASH, TRACK, and SETTINGS; DIAG is not present.

## Tiles and layout

Tap a visible tile once to open its editor. Choose the parameter, visibility,
decimal places, and optional warning. Save writes that one tile transaction to
ESP32 NVS before the runtime layout changes. Cancel and failed writes leave the
active configuration unchanged.

Hidden tiles can be reopened from the complete DASH and TRACK slot lists in
SETTINGS. Hiding a tile compacts only its own logical group toward the bottom:
left, center-wide, center-small, or right on DASH; left, center-wide, or right
on TRACK. Wide tile labels and values are centered.

## Warnings

Each tile independently supports above/below direction, native threshold,
hysteresis, and delay. A breached warning produces a large red WARNING modal
with the current value and configured limit. Acknowledging removes the modal,
but the related visible tile stays red until the value returns through the safe
hysteresis boundary. A hidden tile can still raise its warning. Invalid data
does not raise a warning.

## Global settings

- Brightness uses a software overlay from 20 to 100 percent; there is no off
  switch.
- Source is explicitly CAN or DEMO. CAN never automatically falls back to DEMO.
- CAN is receive-only. Available bitrates are 125, 250, 500, and 1000 kbit/s;
  timeout is adjustable from 100 to 5000 ms.
- The current safe profile is `none`, so no unverified ECU frames are decoded.
- Shift start, red zone, and maximum RPM are shared by DASH and TRACK and must
  satisfy `start < red <= maximum`.
- Temperature, pressure, speed, and mixture units affect presentation only.
  Stored telemetry and warning comparisons remain in native units.
- Reset Layouts restores only tile layouts. Factory Reset clears only the
  `diy_dash` application namespace and restores all defaults.

## Build and hardware validation

GitHub Actions runs native tests, compiles the exact Waveshare target, produces
a merged full-flash BIN and component binaries, and renders deterministic UI
review images. Those checks prove compilation and packaging, not electrical,
touch, CAN-bus, thermal, or long-duration behavior. Complete the physical
acceptance checklist in the README after flashing a test board.
