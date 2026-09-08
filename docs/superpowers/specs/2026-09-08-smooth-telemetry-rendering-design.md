# Smooth Telemetry Rendering Design

## Goal

Make changing telemetry values and the shift-light strip feel consistently smooth on the Waveshare ESP32-S3 Touch LCD 5 without delaying raw CAN warnings or shift-light activation.

## Timing model

The supported board profile uses a 16 MHz RGB pixel clock with an 820 by 500 total timing frame, yielding approximately 39 physical scans per second. LVGL therefore renders dirty regions every 25 ms (40 Hz target). Increasing application updates beyond the physical scan rate is avoided.

Four independent time domains are used:

- CAN ingestion remains event-driven in the main loop.
- Shift-light and warning logic runs every 5 ms (200 Hz) from raw telemetry.
- LVGL dirty-region rendering targets 25 ms (40 Hz).
- Tile presentation is parameter-aware: 25 ms for fast signals, 50 ms for medium signals, and 100 ms for slow signals.

Deadlines advance from their prior scheduled time. A late iteration skips missed periods instead of scheduling the next iteration relative to the late completion time, preventing drift and catch-up bursts.

## Tile presentation

Fast signals include RPM, speed, MAP/boost, throttle/accelerator position, mass airflow, injector pulse width, and wheel speeds. Medium signals include lambda, pressures, ignition timing, injector duty, and gear. Temperatures, battery voltage, ethanol content, and exhaust temperatures are slow signals.

Each tile keeps a small presentation state in RAM. A valid raw value becomes the target; the displayed value approaches it using elapsed-time interpolation. Invalid values reset the presentation immediately. Interpolation affects only the visible number. Warning evaluation and shift-light activation always use the latest raw `VehicleState`.

Each tile also caches its last formatted value, unit, and warning-border state. LVGL setters are called only when the corresponding visible result changes. Configuration or layout changes invalidate these caches.

## Shift light

The shift-light model is evaluated independently every 5 ms. Flash phase is derived from absolute monotonic time, preserving an 8 Hz full-cycle frequency without accumulated timing drift. The active page owns one shallow custom-drawn strip object; a state change invalidates that strip once, rather than mutating twelve LVGL child objects.

At 8 Hz, a complete cycle lasts 125 ms and each red/off phase lasts approximately 62.5 ms. The physical panel can show roughly two to three scans per phase. Peak-RPM hold remains a display concern and never feeds the shift-light model.

## Verification

Native tests verify deadline behavior, parameter cadence classes, interpolation and reset behavior, and the existing exact 8 Hz phase contract. GitHub Actions is the only environment used to run tests and build firmware. The board build additionally compiles the LVGL custom widget and produces the flashable merged binary.

Hardware acceptance after flashing requires stable menu interaction, visually even value motion, an independent 8 Hz strip above the flash threshold, immediate warning response, and no display corruption. Runtime timing instrumentation will expose missed render and shift deadlines for board-side diagnosis.
