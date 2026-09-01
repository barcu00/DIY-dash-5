# BartzDash v0.1 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Build and CI-verify a mock-telemetry motorsport dashboard for Waveshare ESP32-S3-Touch-LCD-5 (800x480).

**Architecture:** PlatformIO provides a native host-test environment and a Waveshare ESP32-S3 firmware environment. Pure telemetry logic is hardware-independent. Board bring-up uses ESP32_Display_Panel and LVGL 8.4; UI reads only `VehicleState`.

**Tech Stack:** PlatformIO, Arduino-ESP32, ESP32_Display_Panel, LVGL 8.4, C++17, GitHub Actions.

**Spec:** `docs/superpowers/specs/2026-09-01-bartzdash-v0.1-design.md`

## Global Constraints
- Target board: Waveshare ESP32-S3-Touch-LCD-5 non-B, 800x480.
- Use `BOARD_WAVESHARE_ESP32_S3_TOUCH_LCD_5` supported-board profile.
- LVGL major version is 8; target 8.4.x.
- v0.1 data source is MOCK only.
- CAN is explicitly disabled in v0.1.
- UI must not contain CAN scaling or hardware driver logic.

---

### Task 1: Native mock telemetry RED test

**Files:**
- Create: `platformio.ini`
- Create: `src/telemetry/vehicle_state.h`
- Create: `src/telemetry/mock_telemetry.h`
- Create: `test/test_mock_telemetry/test_main.cpp`
- Create: `.github/workflows/build.yml`

**Interfaces:**
- Produces: `VehicleState`, `MockTelemetry::reset()`, `MockTelemetry::update(uint32_t)` and `MockTelemetry::state() const`.

- [ ] Write tests asserting reset values, deterministic output, and engineering-range bounds.
- [ ] Push without implementation and verify native CI fails because `MockTelemetry` behavior is missing.
- [ ] Record the expected RED result before production implementation.

### Task 2: Mock telemetry GREEN implementation

**Files:**
- Create: `src/telemetry/mock_telemetry.cpp`

**Interfaces:**
- Implements the Task 1 API without Arduino dependencies.

- [ ] Implement bounded deterministic periodic telemetry.
- [ ] Run native test in CI.
- [ ] Verify all host tests pass.
- [ ] Commit the green implementation.

### Task 3: Firmware board bring-up

**Files:**
- Create: `src/board/board_display.h`
- Create: `src/board/board_display.cpp`
- Create: `src/main.cpp`
- Modify: `platformio.ini`
- Create: `src/lv_conf.h`
- Create: `esp_panel_board_supported_conf.h`

**Interfaces:**
- Produces: `BoardDisplay::begin()`, `BoardDisplay::loop()`, touch/display status, PSRAM diagnostics.

- [ ] Configure 16 MB flash and OPI PSRAM.
- [ ] Configure ESP32_Display_Panel supported board macro for the non-B 5-inch model.
- [ ] Initialize panel, GT911 and LVGL.
- [ ] Allocate LVGL drawing buffers with PSRAM preference.
- [ ] Compile firmware in CI and fix all API/version mismatches.

### Task 4: Motorsport UI

**Files:**
- Create: `src/ui/ui.h`
- Create: `src/ui/ui.cpp`
- Create: `src/app/app.h`
- Create: `src/app/app.cpp`
- Modify: `src/main.cpp`

**Interfaces:**
- `Ui::begin()` creates DASH/DIAG/TRACK.
- `Ui::update(const VehicleState&, const RuntimeDiagnostics&)` updates widgets.
- `App::begin()` and `App::loop()` coordinate the runtime.

- [ ] Create black high-contrast theme.
- [ ] Implement top shift-light bar.
- [ ] Implement DASH values and large RPM/gear.
- [ ] Implement DIAG runtime status with CAN = DISABLED.
- [ ] Implement TRACK reduced telemetry.
- [ ] Add touch navigation DASH/DIAG/TRACK.
- [ ] Compile in CI.

### Task 5: CI artifacts and verification

**Files:**
- Modify: `.github/workflows/build.yml`
- Create: `README.md`

- [ ] Run native tests first.
- [ ] Build `waveshare_5` environment.
- [ ] Upload `firmware.bin`, `bootloader.bin`, and `partitions.bin` if generated.
- [ ] Inspect workflow jobs/logs.
- [ ] Fix build failures until the latest run is green.
- [ ] Verify workflow artifacts exist.
- [ ] Document flashing and hardware acceptance steps without claiming hardware validation.
