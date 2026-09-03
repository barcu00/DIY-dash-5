# DIY Dash Firmware Bootstrap Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Produce tested ESP32-S3 dashboard firmware with real Waveshare display/touch/PSRAM/TWAI initialization, a neutral decoder framework, clearly separated DEMO data, alarms, CI, and flashable artifacts.

**Architecture:** A target-only TWAI adapter yields neutral `CanFrame` objects to a host-testable table-driven `EcuCanDecoder`. Decoded CAN and generated DEMO values remain in separate `VehicleState` snapshots; `TelemetryManager` selects one snapshot for the alarm manager and LVGL UI. `App` coordinates hardware and pure logic without allowing the UI to inspect frames.

**Tech Stack:** PlatformIO, C++17, Arduino-ESP32 for ESP32-S3, ESP32_Display_Panel, ESP32_IO_Expander, LVGL 8.4, ESP-IDF TWAI driver, Unity native tests, GitHub Actions.

**Spec:** `docs/superpowers/specs/2026-09-02-diy-dash-firmware-bootstrap-design.md`

## Global Constraints

- Target Waveshare ESP32-S3-Touch-LCD-5 non-B, 800x480, 16 MB flash, 8 MB OPI PSRAM.
- TWAI TX is GPIO15 and RX is GPIO16.
- The default decoder profile contains zero assumed ECUMaster definitions.
- UI reads only `VehicleState`; raw CAN types and scaling stay outside UI code.
- DEMO and CAN values never coexist in one active snapshot.
- Alarm thresholds are centralized outside UI implementation.
- Dependency versions and PlatformIO are pinned.
- The full image is named `DIY-Dash-ESP32-S3-Touch-LCD-5-full.bin`.
- GitHub Actions artifact is named `DIY-Dash-firmware`.
- Existing working LCD, GT911, PSRAM, and UI elements are retained where compatible.
- No prohibited legacy identifier may remain in tracked source, documentation, workflow, artifact, or new commit metadata.

## File Map

Create:

```text
src/config/dashboard_config.h             central timeouts, bitrate and alarms
src/can/can_frame.h                       hardware-independent received frame
src/can/can_driver.h/.cpp                 ESP32 TWAI adapter
src/ecu/ecu_can_decoder.h/.cpp            table-driven decoder
src/telemetry/telemetry_manager.h/.cpp    CAN/DEMO state selection
src/alarms/alarm_manager.h/.cpp           pure alarm evaluation
test/test_vehicle_state/test_main.cpp
test/test_ecu_can_decoder/test_main.cpp
test/test_telemetry_manager/test_main.cpp
test/test_alarm_manager/test_main.cpp
.github/workflows/build-firmware.yml
```

Modify:

```text
src/telemetry/vehicle_state.h
src/telemetry/mock_telemetry.h/.cpp
src/app/app.h/.cpp
src/board/board_display.h/.cpp
src/ui/ui.h/.cpp
src/main.cpp
src/lv_conf.h
platformio.ini
scripts/merge_bin.py
boards/waveshare_esp32_s3_touch_lcd_5.json
README.md
.gitignore
```

Remove:

```text
.github/workflows/build.yml
```

---

### Task 1: VehicleState and Decoder Core

**Files:**
- Create: `src/can/can_frame.h`
- Create: `src/ecu/ecu_can_decoder.h`
- Create: `src/ecu/ecu_can_decoder.cpp`
- Modify: `src/telemetry/vehicle_state.h`
- Test: `test/test_vehicle_state/test_main.cpp`
- Test: `test/test_ecu_can_decoder/test_main.cpp`
- Modify: `platformio.ini`

**Interfaces:**
- Produces: `VehicleState::set(VehicleSignal, float, uint32_t)`, `VehicleState::get(VehicleSignal)`, `VehicleState::invalidateStale(uint32_t, const uint32_t*)`, `EcuCanDecoder::decode(const CanFrame&, VehicleState&, uint32_t) const`.
- Produces: `SignalDefinition { can_id, extended, byte_offset, byte_order, raw_type, scale, bias, signal, unit, timeout_ms }`.

- [ ] **Step 1: Write failing VehicleState behavior tests**

```cpp
TEST_CASE("new state exposes invalid signals", "[vehicle]") {
    VehicleState state;
    TEST_ASSERT_FALSE(state.get(VehicleSignal::Rpm).valid);
}

TEST_CASE("source reset prevents mixed snapshots", "[vehicle]") {
    VehicleState state;
    state.reset(DataSource::Can);
    state.set(VehicleSignal::Rpm, 4200.0f, 100U);
    state.reset(DataSource::Demo);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(DataSource::Demo), static_cast<uint8_t>(state.source()));
    TEST_ASSERT_FALSE(state.get(VehicleSignal::Rpm).valid);
}
```

- [ ] **Step 2: Run the focused tests and verify RED**

Run: `pio test -e native -f test_vehicle_state`

Expected: compilation fails because `VehicleSignal`, `DataSource`, and the methods do not exist.

- [ ] **Step 3: Implement the minimal state model**

Use fixed-size storage indexed by a contiguous enum:

```cpp
enum class DataSource : uint8_t { None, Can, Demo };
enum class VehicleSignal : uint8_t {
    Rpm, Map, Lambda, Tps, Clt, Iat, OilPressure, OilTemperature,
    BatteryVoltage, Speed, Gear, FuelPressure, Count
};

struct SignalValue {
    float value = 0.0f;
    uint32_t updated_ms = 0U;
    bool valid = false;
};
```

`reset(source)` clears every `SignalValue` before assigning the source.

- [ ] **Step 4: Run VehicleState tests and verify GREEN**

Run: `pio test -e native -f test_vehicle_state`

Expected: all cases pass.

- [ ] **Step 5: Write failing decoder tests with hand-derived fixtures**

```cpp
TEST_CASE("decoder applies little endian scale and bias", "[decoder]") {
    const SignalDefinition defs[] = {
        {0x321, false, 1, ByteOrder::Little, RawType::Unsigned16,
         0.5f, -10.0f, VehicleSignal::Rpm, "rpm", 250U},
    };
    EcuCanDecoder decoder(defs, 1);
    VehicleState state;
    state.reset(DataSource::Can);
    CanFrame frame{0x321, 3, {0x00, 0x34, 0x12}, false, false};
    TEST_ASSERT_TRUE(decoder.decode(frame, state, 50U));
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 2320.0f, state.get(VehicleSignal::Rpm).value);
}
```

Add independent cases for big-endian signed 16-bit data, unsigned 8/32-bit data,
short DLC rejection, standard/extended mismatch, unmapped frame rejection, and
an empty profile accepting nothing.

- [ ] **Step 6: Run decoder tests and verify RED**

Run: `pio test -e native -f test_ecu_can_decoder`

Expected: compilation fails because decoder types are missing.

- [ ] **Step 7: Implement bounds-checked decoding**

Decode raw integers only after verifying `byte_offset + raw_width <= dlc`. Sign
extend signed types through their fixed-width integer type. Return `true` only
when at least one matching definition updates the state. Export per-signal
timeouts from the active profile without embedding any real ECU IDs.

- [ ] **Step 8: Run the focused and full native suites**

Run:

```text
pio test -e native -f test_ecu_can_decoder
pio test -e native
```

Expected: decoder tests and the existing mock telemetry tests pass.

- [ ] **Step 9: Commit the tested core**

Commit: `feat: add neutral vehicle state and CAN decoder core`

---

### Task 2: Telemetry Selection, DEMO Isolation, and Alarms

**Files:**
- Create: `src/config/dashboard_config.h`
- Create: `src/telemetry/telemetry_manager.h`
- Create: `src/telemetry/telemetry_manager.cpp`
- Create: `src/alarms/alarm_manager.h`
- Create: `src/alarms/alarm_manager.cpp`
- Modify: `src/telemetry/mock_telemetry.h`
- Modify: `src/telemetry/mock_telemetry.cpp`
- Test: `test/test_telemetry_manager/test_main.cpp`
- Test: `test/test_alarm_manager/test_main.cpp`

**Interfaces:**
- Consumes: neutral decoder and `VehicleState` from Task 1.
- Produces: `TelemetryManager::accept(const CanFrame&, uint32_t)`, `TelemetryManager::update(uint32_t)`, `TelemetryManager::state()`, `TelemetryManager::canStatus()`.
- Produces: `AlarmManager::evaluate(const VehicleState&) -> AlarmSummary`.

- [ ] **Step 1: Write failing state-selection tests**

Use a one-signal test decoder profile and assert:

```cpp
manager.accept(valid_frame, 100U);
manager.update(150U);
TEST_ASSERT_EQUAL(DataSource::Can, manager.state().source());
manager.update(701U);
TEST_ASSERT_EQUAL(DataSource::Demo, manager.state().source());
TEST_ASSERT_TRUE(manager.demoActive());
```

Add a separate manager with DEMO disabled and assert the same timeout produces
`DataSource::None` and invalid RPM. Assert an unmapped frame never marks CAN
online.

- [ ] **Step 2: Run telemetry manager tests and verify RED**

Run: `pio test -e native -f test_telemetry_manager`

Expected: compilation fails because `TelemetryManager` does not exist.

- [ ] **Step 3: Implement two independent snapshots**

Maintain `can_state_` and `demo_state_`; expose one by const reference. Valid
decoder traffic resets/updates only `can_state_`. Timeout selects a fully reset
and regenerated `demo_state_` when enabled, or a reset `none_state_` otherwise.
CAN state is `InitFailed`, `Waiting`, `Online`, or `Offline` independently of the
active data source.

- [ ] **Step 4: Adapt MockTelemetry to populate VehicleState**

Replace direct public member writes with `reset(DataSource::Demo)` and `set()`.
Keep the existing deterministic wave ranges so the existing test remains useful.

- [ ] **Step 5: Run telemetry and mock tests and verify GREEN**

Run: `pio test -e native -f test_telemetry_manager -f test_mock_telemetry`

Expected: all cases pass.

- [ ] **Step 6: Write failing alarm behavior tests**

Construct literal snapshots for high CLT, high IAT, low oil pressure, low battery,
and load-gated lean lambda. Prove an invalid lambda does not alarm and a lean
lambda below the configured MAP/TPS load threshold does not alarm.

- [ ] **Step 7: Run alarm tests and verify RED**

Run: `pio test -e native -f test_alarm_manager`

Expected: compilation fails because alarm types do not exist.

- [ ] **Step 8: Implement centralized alarm evaluation**

Put thresholds in `dashboard_config.h`. Return a bitmask and highest severity;
do not mutate state and do not include LVGL headers.

- [ ] **Step 9: Run all native tests and commit**

Run: `pio test -e native`

Expected: all tests pass. Commit: `feat: add isolated demo fallback and alarms`.

---

### Task 3: Real TWAI and Application Integration

**Files:**
- Create: `src/can/can_driver.h`
- Create: `src/can/can_driver.cpp`
- Modify: `src/app/app.h`
- Modify: `src/app/app.cpp`
- Modify: `src/main.cpp`
- Modify: `platformio.ini`

**Interfaces:**
- Produces: `CanDriver::begin(uint32_t)`, `CanDriver::poll(CanFrame&)`, `CanDriver::status()`, `CanDriver::receivedFrames()`, `CanDriver::rejectedFrames()`.
- Consumes: `TelemetryManager` and `AlarmManager` from Task 2.

- [ ] **Step 1: Implement target-only TWAI adapter**

Use `TWAI_GENERAL_CONFIG_DEFAULT(GPIO_NUM_15, GPIO_NUM_16, TWAI_MODE_NORMAL)`,
RX queue length 32, TX queue length 0, accept-all filter, and a non-blocking
`twai_receive(..., 0)`. Support 125, 250, 500, and 1000 kbit/s timing macros;
the central default is 1000 kbit/s but remains easy to change.

- [ ] **Step 2: Wire App without defining ECU mappings**

Construct `EcuCanDecoder(nullptr, 0)` for the shipped default. During each loop,
drain a bounded number of frames, pass them to `TelemetryManager`, update timeout
state, evaluate alarms, and update UI at 20 Hz. A failed CAN init must not stop
display/UI startup.

- [ ] **Step 3: Add startup logging**

Print neutral project prefix, flash bytes, PSRAM bytes, LCD dimensions, touch
status, CAN pins/bitrate, decoder mapping count, and whether DEMO fallback is
enabled.

- [ ] **Step 4: Compile the target**

Run: `pio run -e waveshare_5`

Expected: compilation and link exit 0. If it fails, invoke systematic debugging,
identify the first root cause, add a regression test where pure logic is involved,
and retry.

- [ ] **Step 5: Run the native suite again and commit**

Run: `pio test -e native`

Expected: all tests pass. Commit: `feat: initialize ESP32-S3 TWAI dashboard flow`.

---

### Task 4: LCD/LVGL/UI and Alarm Presentation

**Files:**
- Modify: `src/board/board_display.h`
- Modify: `src/board/board_display.cpp`
- Modify: `src/ui/ui.h`
- Modify: `src/ui/ui.cpp`
- Modify: `src/lv_conf.h`

**Interfaces:**
- Consumes: `VehicleState`, `CanStatus`, `AlarmSummary`, and diagnostics.
- Produces: real 800x480 DASH plus lightweight TRACK, DIAG, and SETTINGS pages.

- [ ] **Step 1: Harden board diagnostics**

Retain `ESP32_Display_Panel` and GT911 access. Reject a non-800x480 panel,
report PSRAM allocation/fallback explicitly, use a neutral timer name, and remove
all prohibited source identifiers.

- [ ] **Step 2: Adapt UI to validity-aware values**

Render `---` for invalid values. DASH must display RPM, MAP/boost, lambda, TPS,
CLT, IAT, oil pressure, oil temperature, battery voltage, and CAN/source badge.
Format RPM with no decimals, lambda/battery with two, and temperatures with none.

- [ ] **Step 3: Apply alarm styling and source disclosure**

Use centralized alarm results to color affected tiles amber/red. Show `DEMO`
prominently whenever the active snapshot is generated. Show CAN state even while
DEMO is active.

- [ ] **Step 4: Preserve modular future screens**

Keep TRACK and DIAG usable, add a minimal SETTINGS page with read-only bitrate,
timeout, DEMO state, and decoder mapping count, and isolate screen creation from
the update path for future SquareLine-generated files.

- [ ] **Step 5: Build and test**

Run:

```text
pio test -e native
pio run -e waveshare_5
```

Expected: both commands exit 0. Commit: `feat: render CAN-aware motorsport dashboard`.

---

### Task 5: Reproducible Binaries, CI, and Documentation

**Files:**
- Modify: `scripts/merge_bin.py`
- Modify: `platformio.ini`
- Create: `.github/workflows/build-firmware.yml`
- Remove: `.github/workflows/build.yml`
- Modify: `.gitignore`
- Modify: `README.md`

**Interfaces:**
- Consumes: PlatformIO `FLASH_EXTRA_IMAGES`, `ESP32_APP_OFFSET`, and target build.
- Produces: validated raw and merged firmware artifact files.

- [ ] **Step 1: Pin build dependencies**

Resolve current repository tags/commits with `git ls-remote` and pin
`ESP32_Display_Panel`, `ESP32_IO_Expander`, `esp-lib-utils`, and LVGL 8.4.x in
`platformio.ini`. Keep the existing pinned pioarduino platform unless the target
build proves an incompatibility.

- [ ] **Step 2: Make merging metadata-driven**

Retain SCons-derived `FLASH_EXTRA_IMAGES` and `ESP32_APP_OFFSET`; rename the
output to `DIY-Dash-ESP32-S3-Touch-LCD-5-full.bin`. Emit a JSON manifest with the
exact ordered address/file pairs used by esptool. Fail when the application image
or any declared extra image is missing.

- [ ] **Step 3: Create the workflow**

Use push, pull request, and `workflow_dispatch`; Python 3.12; pinned PlatformIO;
cache `~/.platformio/.cache`, packages, and platforms keyed by `platformio.ini`;
run native tests before target build; copy all expected binaries plus manifest;
require firmware, bootloader, partitions, and merged image; conditionally copy
`boot_app0.bin`; upload as `DIY-Dash-firmware` with `if-no-files-found: error`.

- [ ] **Step 4: Complete README and ignore rules**

Document hardware, 800x480, flash/PSRAM, GPIO15/GPIO16, local commands, direct
and merged flashing, CI artifacts, architecture/data flow, `VehicleState`, generic
decoder schema, empty ECUMaster mapping, DEMO behavior, alarms, and physical
acceptance limits. Ignore `.pio`, editor files, OS metadata, cache, logs, and
local binaries.

- [ ] **Step 5: Verify artifact generation locally**

Run: `pio run -e waveshare_5`

Assert `.pio/build/waveshare_5/` contains `firmware.bin`, `bootloader.bin`,
`partitions.bin`, the merged image, and merge manifest; record whether
`boot_app0.bin` is generated by the selected framework.

- [ ] **Step 6: Run native tests and commit**

Run: `pio test -e native`

Expected: all tests pass. Commit: `ci: build and publish DIY Dash firmware`.

---

### Task 6: Hygiene, Single Requested Commit, Push, PR, and Actions

**Files:**
- Inspect all tracked files and generated outputs.

**Interfaces:**
- Produces: the requested branch, final commit, PR, successful workflow, and
verified downloadable artifact inventory.

- [ ] **Step 1: Run fresh local verification**

Run:

```text
pio test -e native
pio run -e waveshare_5
git diff --check origin/main...HEAD
git status --short --branch
git ls-files
```

Expected: zero failed tests, build exit 0, no whitespace errors, and no generated
`.pio`, cache, credentials, tokens, or binaries tracked.

- [ ] **Step 2: Scan naming and secrets**

Search tracked content and paths for the prohibited legacy identifier. Search
for common token/private-key patterns and inspect every hit manually. The design
and plan describe the rule without embedding the identifier itself.

- [ ] **Step 3: Consolidate local implementation commits**

After verification, replace the local task commits since `origin/main` with one
commit containing the complete reviewed tree and exact message:

```text
Bootstrap ESP32-S3 dashboard for Touch-LCD-5 with CI
```

Verify its tree matches the pre-consolidation tree and its parent is
`origin/main` before pushing.

- [ ] **Step 4: Push `dashboard-dev`**

Use the repository's authenticated GitHub transport. Do not store credentials in
the repo or command history. Confirm `origin/dashboard-dev` resolves to local HEAD.

- [ ] **Step 5: Open the pull request**

Title: `ESP32-S3 dashboard initial implementation`.

Describe hardware, LCD/touch, PSRAM, TWAI pins, `VehicleState`, generic decoder,
UI, alarms, workflow, artifacts, and the deliberately missing verified
ECUMaster definitions.

- [ ] **Step 6: Wait for GitHub Actions and inspect results**

Require every check in `.github/workflows/build-firmware.yml` to conclude
`success`. On failure, read the complete failing step, reproduce locally when
possible, apply one root-cause fix, recommit with the required final message, push,
and wait again.

- [ ] **Step 7: Verify remote artifact inventory**

Download or inspect the successful run artifact and report exact filenames and
sizes. Confirm the merged image uses the required name.

- [ ] **Step 8: Report evidence**

Return branch, final hash, PR URL if created, workflow name/status, artifact name
and contents, local test/build results, implemented changes, and the next-step
ECUMaster mapping/physical hardware work.
