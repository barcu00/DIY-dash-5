# Smooth Telemetry Rendering Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Deliver stable 40 Hz dirty-region rendering, parameter-aware tile presentation, and an independent 200 Hz raw-RPM shift-light path.

**Architecture:** Pure native-testable cadence and presentation components determine deadlines and displayed values. `App` owns independent render and shift schedules, while LVGL views cache visible state and invalidate only changed regions.

**Tech Stack:** C++17, Arduino ESP32-S3, LVGL 8.4, PlatformIO, Unity, GitHub Actions

**Spec:** `docs/superpowers/specs/2026-09-08-smooth-telemetry-rendering-design.md`

## Global Constraints

- Run all tests and firmware builds only in GitHub Actions.
- Target the Waveshare ESP32-S3 Touch LCD 5 at 800x480 RGB565.
- Render dirty regions at 25 ms, evaluate shift logic at 5 ms, and keep warnings and shift activation on raw telemetry.
- Preserve the configured 4 Hz shift-light flash and the existing 2-second peak-RPM hold.
- Do not raise the board's official 16 MHz RGB pixel clock.

---

### Task 1: Deterministic cadence and tile presentation

**Files:**
- Create: `src/ui/frame_scheduler.h`
- Create: `src/ui/frame_scheduler.cpp`
- Create: `src/ui/tile_refresh_policy.h`
- Create: `src/ui/tile_refresh_policy.cpp`
- Create: `src/ui/display_signal_filter.h`
- Create: `src/ui/display_signal_filter.cpp`
- Create: `test/test_frame_scheduler/test_main.cpp`
- Create: `test/test_tile_refresh_policy/test_main.cpp`
- Create: `test/test_display_signal_filter/test_main.cpp`
- Modify: `platformio.ini`

**Interfaces:**
- Produces: `FrameScheduler::reset(uint32_t)`, `takeRender(uint32_t)`, and `takeShift(uint32_t)`.
- Produces: `tileRefreshIntervalMs(ParameterId)` returning 25, 50, or 100 ms.
- Produces: `DisplaySignalFilter::sample(const SignalValue&, uint32_t, uint32_t)` returning a visible `SignalValue`.

- [ ] **Step 1: Write failing native tests**

  Assert 5 ms shift deadlines, 25 ms render deadlines, skipped late periods without drift, representative parameter classes, smooth bounded movement toward a changed target, and immediate invalid reset.

- [ ] **Step 2: Push the tests and verify RED in GitHub Actions**

  Expected: the native job fails because the three production interfaces do not exist yet.

- [ ] **Step 3: Implement the minimal pure C++ components**

  Advance deadlines from scheduled timestamps, classify every `ParameterId`, and use elapsed-time first-order interpolation with immediate first-value and invalid transitions.

- [ ] **Step 4: Push and verify the pure components in GitHub Actions**

  Expected: all native tests pass and the board build remains green.

### Task 2: Integrate independent rendering and efficient LVGL views

**Files:**
- Modify: `src/app/app.h`
- Modify: `src/app/app.cpp`
- Modify: `src/ui/ui.h`
- Modify: `src/ui/ui.cpp`
- Modify: `src/ui/tile_view.h`
- Modify: `src/ui/tile_view.cpp`
- Modify: `src/ui/shift_light_view.h`
- Modify: `src/ui/shift_light_view.cpp`
- Modify: `src/board/board_display.h`
- Modify: `src/board/board_display.cpp`
- Modify: `platformio.ini`

**Interfaces:**
- Consumes: all Task 1 interfaces.
- Produces: `Ui::updateShiftLight(const VehicleState&, uint32_t, const ShiftLightConfig&)`.

- [ ] **Step 1: Add integration-facing failing tests**

  Extend cadence and policy tests to cover late-loop behavior and every parameter ID; retain the existing shift-model phase boundary tests.

- [ ] **Step 2: Push and verify RED in GitHub Actions**

  Expected: tests fail on missing integration behavior or the old 20 ms render contract.

- [ ] **Step 3: Integrate cadence, interpolation, and visible-state caching**

  Run warnings and shift at 5 ms, render the active data page at 25 ms, pass timestamps into tiles, and call LVGL setters only for changed text, units, or warning state.

- [ ] **Step 4: Replace the twelve shift objects with one custom-drawn strip**

  Cache `ShiftSegmentStates`, invalidate the strip once on state changes, and draw all twelve segments in its `LV_EVENT_DRAW_MAIN` callback.

- [ ] **Step 5: Configure LVGL and runtime metrics**

  Define `LV_DISP_DEF_REFR_PERIOD=25` for the board build and count late render/shift deadlines in diagnostics without changing the pixel clock.

- [ ] **Step 6: Push and verify GREEN in GitHub Actions**

  Expected: native tests, packaging tests, firmware build, UI previews, and artifact collection all pass.

- [ ] **Step 7: Download and identify the merged firmware**

  Download the final workflow artifact and report its absolute path, size, and SHA-256 for the physical board test.
