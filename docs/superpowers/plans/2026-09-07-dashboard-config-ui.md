# Dashboard Configuration and UI Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Build the persistent configurable DASH/TRACK UI, shared shift-light strip, tile editor, settings, unit presentation, and per-tile warning system defined in the approved design.

**Architecture:** Pure C++ models own parameters, configuration, layout packing, unit conversion, shift-light progression, editor staging, and warning state. LVGL views render those models and forward user actions; ESP32-only adapters own NVS and display dimming. `App` coordinates explicit CAN/DEMO selection and applies configuration changes without putting persistence, ECU mappings, or warning policy in UI widgets.

**Tech Stack:** C++17, PlatformIO 6.1.18, Arduino-ESP32, LVGL 8.4.0, ESP32 Preferences/NVS, Unity native tests, GitHub Actions.

**Spec:** `docs/superpowers/specs/2026-09-07-configurable-dashboard-can-profiles-design.md`

## Global Constraints

- Target Waveshare ESP32-S3-Touch-LCD-5 non-B at 800x480.
- Keep the existing board, touch, PSRAM, TWAI, packaging, and artifact implementation intact unless a task below explicitly changes it.
- The only runtime sources are `CAN` and `DEMO`; never fall back automatically from CAN to DEMO.
- Remove DIAG completely; navigation is exactly `DASH | TRACK | SETTINGS`.
- DASH and TRACK use the same shift-light appearance and the same persisted start, red-zone, and maximum RPM values.
- Brightness is software dimming constrained to 20-100%; there is no off control.
- Every tile save is independent and becomes visible only after persistence succeeds.
- Unit conversion is presentation-only; telemetry and warning thresholds remain in native units.
- Do not add ECU frame mappings in this plan. CAN profile implementation follows in a source-audited plan.
- Do not run native tests or firmware builds locally. Every red/green verification step is executed by GitHub Actions after pushing `dashboard-dev`.
- Do not copy protocol or UI code wholesale from the experimental remote branch; port only small model ideas that conform to the approved design.

## Plan boundaries

The approved specification spans three independently reviewable subsystems. This
is plan 1 of 3 and delivers a working UI/configuration subsystem using the
existing empty decoder and DEMO values. Follow-up plans are:

1. verified standalone ECU profiles and source-pinned decoder fixtures;
2. experimental 2005 C2 engine-CAN profile, final CI artifact inspection, and
   final UI/vehicle validation documentation.

The split prevents unverified protocol research from blocking the UI and avoids
combining a large visual rewrite with safety-sensitive reverse-engineered CAN
mappings.

## File structure

Create focused pure-model files:

```text
src/telemetry/parameter_id.h             stable parameter and unit enums
src/telemetry/parameter_registry.h/.cpp  names, native units, defaults
src/settings/app_config.h/.cpp           defaults and validation
src/settings/config_repository.h/.cpp    validated load/save transaction
src/settings/nvs_config_backend.h/.cpp   ESP32 Preferences adapter
src/ui/tile_layout.h                     logical addresses and geometry
src/ui/tile_engine.h/.cpp                group-local bottom packing
src/ui/tile_editor_model.h/.cpp          staged per-slot editing
src/ui/unit_presenter.h/.cpp             value/threshold conversion
src/ui/shift_light_model.h/.cpp          shared RPM-to-segment state
src/alarms/tile_warning_engine.h/.cpp    delay/hysteresis/modal state machine
src/ui/ui_theme.h                        shared LVGL colors and dimensions
src/ui/tile_view.h/.cpp                  reusable tile widget
src/ui/tile_editor_view.h/.cpp           modal editor controls
src/ui/settings_view.h/.cpp              scrollable settings controls
src/ui/warning_modal.h/.cpp              full-screen warning presentation
```

Modify orchestration and existing views:

```text
src/telemetry/vehicle_state.h
src/telemetry/telemetry_manager.h/.cpp
src/can/can_driver.h/.cpp
src/board/board_display.h/.cpp
src/ui/ui.h/.cpp
src/app/app.h/.cpp
platformio.ini
.github/workflows/build-firmware.yml
README.md
```

Create native tests and CI preview support:

```text
test/test_parameter_registry/test_main.cpp
test/test_app_config/test_main.cpp
test/test_tile_engine/test_main.cpp
test/test_config_repository/test_main.cpp
test/test_tile_editor_model/test_main.cpp
test/test_unit_presenter/test_main.cpp
test/test_shift_light_model/test_main.cpp
test/test_tile_warning_engine/test_main.cpp
test/test_source_selection/test_main.cpp
scripts/ui_preview/render_ui_preview.py
scripts/ui_preview/ui_contract.json
scripts/tests/test_ui_preview.py
```

---

### Task 1: Stable parameter registry and unit presentation

**Files:**
- Create: `src/telemetry/parameter_id.h`
- Create: `src/telemetry/parameter_registry.h`
- Create: `src/telemetry/parameter_registry.cpp`
- Create: `src/ui/unit_presenter.h`
- Create: `src/ui/unit_presenter.cpp`
- Modify: `src/telemetry/vehicle_state.h`
- Modify: `src/ecu/ecu_can_decoder.h`
- Modify: `src/ecu/ecu_can_decoder.cpp`
- Modify: `src/telemetry/mock_telemetry.cpp`
- Modify: `platformio.ini`
- Test: `test/test_parameter_registry/test_main.cpp`
- Test: `test/test_unit_presenter/test_main.cpp`

**Interfaces:**
- Produces: `ParameterId`, `NativeUnit`, `ParameterDescriptor`, `parameterDescriptor(ParameterId)`, `UnitSettings`, `PresentedValue`, `UnitPresenter::present(...)`, `UnitPresenter::toNative(...)`.
- Preserves: `using VehicleSignal = ParameterId` so the existing decoder and tests can migrate without a flag-day rename.

- [ ] **Step 1: Add failing registry and conversion tests**

Use these contract examples:

```cpp
void test_registry_describes_all_parameters() {
    for (size_t i = 0; i < parameterCount(); ++i) {
        const auto id = static_cast<ParameterId>(i);
        TEST_ASSERT_NOT_NULL(parameterDescriptor(id).name);
    }
    TEST_ASSERT_EQUAL_STRING("RPM", parameterDescriptor(ParameterId::Rpm).short_name);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(NativeUnit::Rpm),
                            static_cast<uint8_t>(parameterDescriptor(ParameterId::Rpm).native_unit));
}

void test_pressure_and_temperature_round_trip_to_native() {
    UnitSettings settings{};
    settings.temperature = TemperatureUnit::Fahrenheit;
    settings.pressure = PressureUnit::Psi;
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 212.0f,
        UnitPresenter::present(ParameterId::Clt, 100.0f, settings).value);
    const float shown = UnitPresenter::present(ParameterId::OilPressure, 2.0f, settings).value;
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 2.0f,
        UnitPresenter::toNative(ParameterId::OilPressure, shown, settings));
}

void test_lambda_to_afr_uses_configured_stoich() {
    UnitSettings settings{};
    settings.mixture = MixtureUnit::Afr;
    settings.stoich_afr = 14.7f;
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 12.495f,
        UnitPresenter::present(ParameterId::Lambda, 0.85f, settings).value);
}
```

- [ ] **Step 2: Push the red tests to GitHub Actions**

```powershell
git add test/test_parameter_registry test/test_unit_presenter platformio.ini
git commit -m "test: define parameter and unit presentation contracts"
git push origin dashboard-dev
```

Expected GitHub result: `Run native unit tests` fails because the new parameter
and presenter interfaces do not exist. Record the failing run URL in the task
notes; do not run `pio` locally.

- [ ] **Step 3: Implement stable types and descriptors**

Define the enum once and keep its numeric order stable:

```cpp
enum class ParameterId : uint8_t {
    Rpm, Map, Lambda, Tps, Clt, Iat, OilPressure, OilTemperature,
    BatteryVoltage, Speed, Gear, FuelPressure, Count,
};

using VehicleSignal = ParameterId;

enum class NativeUnit : uint8_t {
    Rpm, Bar, Lambda, Percent, Celsius, Volt, Kph, Gear,
};
```

`parameterDescriptor()` returns a descriptor for every value below `Count` and
a fixed `Unknown` descriptor for out-of-range input. Keep all current telemetry
in its existing native units: °C, bar, km/h, lambda, volts, percent, RPM, and
integer gear.

- [ ] **Step 4: Implement reversible presentation conversion**

```cpp
struct UnitSettings {
    TemperatureUnit temperature = TemperatureUnit::Celsius;
    PressureUnit pressure = PressureUnit::Bar;
    SpeedUnit speed = SpeedUnit::Kph;
    MixtureUnit mixture = MixtureUnit::Lambda;
    float stoich_afr = 14.7f;
};

struct PresentedValue {
    float value;
    const char* unit;
};

class UnitPresenter {
public:
    static PresentedValue present(ParameterId id, float native_value,
                                  const UnitSettings& settings);
    static float toNative(ParameterId id, float presented_value,
                          const UnitSettings& settings);
};
```

Use `°F = °C * 9/5 + 32`, `kPa = bar * 100`,
`psi = bar * 14.5037738`, `mph = km/h * 0.621371192`, and
`AFR = lambda * stoich_afr`. Clamp stoichiometric ratio during configuration
validation, not inside conversions.

- [ ] **Step 5: Push the green implementation and verify GitHub Actions**

```powershell
git add src/telemetry src/ecu src/ui/unit_presenter.* platformio.ini test
git commit -m "feat: add stable parameter registry and unit presenter"
git push origin dashboard-dev
```

Expected GitHub result: native registry/unit tests, existing tests, firmware
build, packaging tests, and artifact upload all pass.

### Task 2: Versioned application configuration and approved defaults

**Files:**
- Create: `src/settings/app_config.h`
- Create: `src/settings/app_config.cpp`
- Modify: `platformio.ini`
- Test: `test/test_app_config/test_main.cpp`

**Interfaces:**
- Consumes: `ParameterId`, `UnitSettings` from Task 1.
- Produces: `PageId`, `TileSize`, `TileGroup`, `TileAddress`, `TileConfig`, `TileWarningConfig`, `ShiftLightConfig`, `CanSettings`, `AppConfig::defaults()`, and `AppConfig::validate()`.

- [ ] **Step 1: Write failing configuration tests**

```cpp
void test_defaults_define_every_dash_and_track_slot() {
    const AppConfig config = AppConfig::defaults();
    TEST_ASSERT_EQUAL_UINT8(14U, config.dash_tiles.size());
    TEST_ASSERT_EQUAL_UINT8(12U, config.track_tiles.size());
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(ParameterId::Rpm),
        static_cast<uint8_t>(config.dash_tiles[4].parameter));
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(ParameterId::Gear),
        static_cast<uint8_t>(config.dash_tiles[5].parameter));
}

void test_validation_clamps_ranges_and_rejects_bad_shift_order() {
    AppConfig config = AppConfig::defaults();
    config.brightness_percent = 0U;
    config.units.stoich_afr = 0.0f;
    config.shift = ShiftLightConfig{7000U, 6000U, 5000U};
    const ValidationResult result = config.validate();
    TEST_ASSERT_FALSE(result.shift_order_valid);
    TEST_ASSERT_EQUAL_UINT8(20U, config.brightness_percent);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 14.7f, config.units.stoich_afr);
}
```

- [ ] **Step 2: Push the red configuration contract**

Commit `test: define dashboard configuration defaults`, push
`dashboard-dev`, and confirm the GitHub native-test step fails on missing
`AppConfig`. Do not run local tests.

- [ ] **Step 3: Implement the configuration types**

Use fixed arrays so NVS serialization has a bounded footprint:

```cpp
enum class PageId : uint8_t { Dash, Track, Settings };
enum class TileSize : uint8_t { Small, Wide };
enum class WarningDirection : uint8_t { Above, Below };
enum class TileGroup : uint8_t {
    DashLeft, DashCenterWide, DashCenterSmall, DashRight,
    TrackLeft, TrackCenter, TrackRight,
};

struct TileAddress { PageId page; uint8_t slot; };

struct ShiftLightConfig {
    uint16_t start_rpm = 5500U;
    uint16_t red_rpm = 7000U;
    uint16_t max_rpm = 8000U;
};

struct CanSettings {
    std::array<char, 32> profile_id{};
    uint32_t bitrate = 500000U;
    uint32_t timeout_ms = 500U;
};

struct TileWarningConfig {
    bool enabled = false;
    WarningDirection direction = WarningDirection::Above;
    float threshold_native = 0.0f;
    float hysteresis_native = 0.0f;
    uint16_t delay_ms = 0U;
};

struct TileConfig {
    ParameterId parameter = ParameterId::Rpm;
    bool visible = true;
    uint8_t decimals = 0U;
    TileWarningConfig warning{};
};

struct ValidationResult {
    bool valid = true;
    bool shift_order_valid = true;
};
```

Use 14 DASH slots: four left small, two center wide, four center small, four
right small. Treat the two wide slots as their own bottom-compacting group so
hiding the upper wide tile moves the lower wide tile down. Use 12 TRACK slots:
four left small, four center wide, four right small.

Set `AppConfig::kSchemaVersion` to `1U` for this new serialized shape. Defaults
are source `DEMO`, brightness 100, CAN bitrate 500000, timeout 500 ms,
shift start 5500 RPM, red zone 7000 RPM, maximum 8000 RPM, metric units, lambda,
and stoichiometric AFR 14.7. The initial profile ID is an empty safe profile.

- [ ] **Step 4: Implement validation without silently fixing shift order**

`AppConfig::validate()` clamps brightness, decimals (0-3), supported bitrate,
timeout (100-5000 ms), non-negative hysteresis, delay (0-10000 ms), and
stoichiometric AFR (fallback 14.7 when outside 5.0-20.0). It returns
`shift_order_valid=false` for invalid RPM ordering and leaves the last valid
shift configuration available to the caller instead of reordering values.

- [ ] **Step 5: Push implementation and verify green CI**

Commit `feat: add versioned dashboard configuration`, push, and require the
complete GitHub workflow to pass.

### Task 3: Deterministic group-local tile layout

**Files:**
- Create: `src/ui/tile_layout.h`
- Create: `src/ui/tile_engine.h`
- Create: `src/ui/tile_engine.cpp`
- Create: `src/ui/shift_light_model.h`
- Create: `src/ui/shift_light_model.cpp`
- Modify: `platformio.ini`
- Test: `test/test_tile_engine/test_main.cpp`
- Test: `test/test_shift_light_model/test_main.cpp`

**Interfaces:**
- Consumes: `AppConfig`, `TileAddress`, `TileGroup`, `ShiftLightConfig`.
- Produces: `TileGeometry`, `TilePlacement`, `TileEngine::placements(...)`, `ShiftSegmentState`, `ShiftLightModel::segments(...)`.

- [ ] **Step 1: Write failing packing tests for every group shape**

```cpp
void test_dash_left_tiles_compact_to_bottom_without_crossing_groups() {
    AppConfig config = AppConfig::defaults();
    config.dash_tiles[0].visible = false;
    const auto result = TileEngine::placements(PageId::Dash, config);
    TEST_ASSERT_EQUAL_UINT8(3U, visibleCount(result, TileGroup::DashLeft));
    TEST_ASSERT_EQUAL_INT16(dashSmallRowY(1), find(result, {PageId::Dash, 1}).y);
    TEST_ASSERT_EQUAL_INT16(dashSmallRowY(3), find(result, {PageId::Dash, 3}).y);
    TEST_ASSERT_EQUAL_UINT8(4U, visibleCount(result, TileGroup::DashRight));
}

void test_dash_wide_tiles_compact_inside_wide_group() {
    AppConfig config = AppConfig::defaults();
    config.dash_tiles[4].visible = false;
    const auto result = TileEngine::placements(PageId::Dash, config);
    TEST_ASSERT_EQUAL_INT16(dashWideRowY(1), find(result, {PageId::Dash, 5}).y);
}

void test_track_center_all_hidden_produces_no_center_placements() {
    AppConfig config = AppConfig::defaults();
    for (uint8_t slot = 4; slot < 8; ++slot) config.track_tiles[slot].visible = false;
    TEST_ASSERT_EQUAL_UINT8(0U,
        visibleCount(TileEngine::placements(PageId::Track, config), TileGroup::TrackCenter));
}
```

- [ ] **Step 2: Write the failing shared shift-light tests**

```cpp
void test_both_pages_use_identical_shift_segments() {
    const ShiftLightConfig config{5500U, 7000U, 8000U};
    const auto dash = ShiftLightModel::segments(7250U, config);
    const auto track = ShiftLightModel::segments(7250U, config);
    TEST_ASSERT_EQUAL_MEMORY(dash.data(), track.data(), sizeof(dash));
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(ShiftColor::Red),
                            static_cast<uint8_t>(dash[7].color));
}
```

- [ ] **Step 3: Push the red model tests and confirm GitHub failure**

Commit `test: define tile packing and shift light behavior`, push, and record
the expected missing-interface failure from GitHub Actions.

- [ ] **Step 4: Implement geometry and bottom packing**

Use an 8 px outer margin, 8 px gaps, shift strip at `y=8`, content beginning at
`y=36`, navigation at `y=430` with height 50, and content ending at `y=422`.
Define geometry centrally in `tile_layout.h`; no LVGL object should contain its
own copy of these coordinates.

`TileEngine::placements(PageId, const AppConfig&)` iterates each group's logical
slots in original order, counts visible entries, assigns them to the lowest
`visible_count` geometry rows, and emits at most 14 placements. Center-small
DASH slots retain their two-column order while moving by complete row and then
within the final row deterministically.

- [ ] **Step 5: Implement one shared 12-segment shift model**

Map RPM below start to zero lit segments, linearly fill segments from start to
maximum, use green before the red-zone boundary, yellow for the two segments
immediately before that boundary, and red from the boundary through maximum.
Clamp RPM above maximum to all segments lit. Both UI pages call this same model.

- [ ] **Step 6: Push green implementation and verify GitHub Actions**

Commit `feat: add grouped tile layout and shared shift lights`, push, and require
all native/model tests plus the target build to pass.

### Task 4: Transactional persistence and staged tile editing

**Files:**
- Create: `src/settings/config_repository.h`
- Create: `src/settings/config_repository.cpp`
- Create: `src/settings/nvs_config_backend.h`
- Create: `src/settings/nvs_config_backend.cpp`
- Create: `src/ui/tile_editor_model.h`
- Create: `src/ui/tile_editor_model.cpp`
- Modify: `platformio.ini`
- Test: `test/test_config_repository/test_main.cpp`
- Test: `test/test_tile_editor_model/test_main.cpp`

**Interfaces:**
- Consumes: `AppConfig`, `TileAddress`, `ParameterId`, `UnitPresenter`.
- Produces: `ConfigBackend`, `ConfigRepository::load/save/reset`, `NvsConfigBackend`, `TileEditorModel::open/cancel/save`, `TileEditorDraft`.

- [ ] **Step 1: Write failing persistence tests using an in-memory backend**

```cpp
class MemoryBackend : public ConfigBackend {
public:
    bool read(void* data, size_t size) override;
    bool write(const void* data, size_t size) override;
    bool erase() override;
    bool fail_writes = false;
};

void test_schema_mismatch_loads_safe_defaults() {
    MemoryBackend backend;
    AppConfig old = AppConfig::defaults();
    old.schema_version = 0U;
    backend.write(&old, sizeof(old));
    AppConfig loaded{};
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(LoadResult::DefaultsUsed),
        static_cast<uint8_t>(ConfigRepository(backend).load(loaded)));
    TEST_ASSERT_EQUAL_UINT32(AppConfig::kSchemaVersion, loaded.schema_version);
}

void test_failed_save_does_not_replace_runtime_config() {
    MemoryBackend backend;
    ConfigRepository repository(backend);
    AppConfig runtime = AppConfig::defaults();
    AppConfig candidate = runtime;
    candidate.brightness_percent = 40U;
    backend.fail_writes = true;
    TEST_ASSERT_FALSE(repository.saveCandidate(candidate, runtime));
    TEST_ASSERT_EQUAL_UINT8(100U, runtime.brightness_percent);
}
```

- [ ] **Step 2: Write failing editor Save/Cancel tests**

```cpp
void test_cancel_leaves_tile_unchanged() {
    AppConfig config = AppConfig::defaults();
    TileEditorModel editor;
    editor.open({PageId::Dash, 0U}, config);
    editor.setVisible(false);
    editor.cancel();
    TEST_ASSERT_TRUE(config.dash_tiles[0].visible);
}

void test_parameter_change_disables_previous_warning_on_save() {
    AppConfig config = AppConfig::defaults();
    config.dash_tiles[0].warning.enabled = true;
    TileEditorModel editor;
    editor.open({PageId::Dash, 0U}, config);
    editor.setParameter(ParameterId::Clt);
    TEST_ASSERT_TRUE(editor.applyTo(config));
    TEST_ASSERT_FALSE(config.dash_tiles[0].warning.enabled);
}
```

- [ ] **Step 3: Push red persistence/editor tests to GitHub**

Commit `test: define persistent tile editing transactions`, push, and verify the
native job fails only because the new repository/editor types are absent.

- [ ] **Step 4: Implement repository validation and NVS adapter**

```cpp
class ConfigBackend {
public:
    virtual ~ConfigBackend() = default;
    virtual bool read(void* data, size_t size) = 0;
    virtual bool write(const void* data, size_t size) = 0;
    virtual bool erase() = 0;
};
```

`ConfigRepository::load()` accepts a blob only when its byte size and schema
version match and `validate()` succeeds; otherwise it returns defaults.
`saveCandidate(candidate, runtime)` validates a copy, writes it, and assigns it
to `runtime` only after the backend confirms success. `NvsConfigBackend` uses
the application-owned namespace `diy_dash` and blob key `config` under
`#ifdef ARDUINO`.

- [ ] **Step 5: Implement one editor draft per open session**

`open()` copies exactly one slot into `TileEditorDraft`. Setters affect only the
draft. `applyTo()` validates address, parameter, decimals, threshold,
hysteresis, and delay; it resets the warning when the parameter differs from
the original. UI code will pass the resulting whole-config candidate through
`ConfigRepository` before applying it to runtime.

- [ ] **Step 6: Push green implementation and verify GitHub Actions**

Commit `feat: persist staged per-tile configuration`, push, and require native,
firmware, packaging, and artifact jobs to pass.

### Task 5: Per-tile warning state machine

**Files:**
- Create: `src/alarms/tile_warning_engine.h`
- Create: `src/alarms/tile_warning_engine.cpp`
- Modify: `src/alarms/alarm_manager.h`
- Modify: `src/alarms/alarm_manager.cpp`
- Modify: `platformio.ini`
- Test: `test/test_tile_warning_engine/test_main.cpp`
- Modify: `test/test_alarm_manager/test_main.cpp`

**Interfaces:**
- Consumes: all DASH/TRACK `TileConfig` records and fresh values from `VehicleState`.
- Produces: `TileAlarmState`, `WarningModalData`, `TileWarningEngine::evaluate`, `nextModal`, `acknowledge`, and `isHighlighted`.

- [ ] **Step 1: Write failing state-transition tests**

Cover above/below direction, exact delay boundary, hysteresis, invalid values,
hidden tiles, acknowledgement, rearming, priority, and ties. The core test uses:

```cpp
void test_acknowledged_alarm_retriggers_only_after_safe_rearm() {
    AppConfig config = oneWarning(ParameterId::Clt, WarningDirection::Above,
                                  110.0f, 5.0f, 200U, false);
    VehicleState state;
    state.reset(DataSource::Can);
    TileWarningEngine engine;
    state.set(ParameterId::Clt, 111.0f, 100U);
    engine.evaluate(config, state, 100U);
    engine.evaluate(config, state, 300U);
    TEST_ASSERT_TRUE(engine.nextModal().has_value());
    engine.acknowledge(engine.nextModal()->address);
    engine.evaluate(config, state, 400U);
    TEST_ASSERT_FALSE(engine.nextModal().has_value());
    state.set(ParameterId::Clt, 104.0f, 500U);
    engine.evaluate(config, state, 500U);
    state.set(ParameterId::Clt, 111.0f, 600U);
    engine.evaluate(config, state, 800U);
    TEST_ASSERT_TRUE(engine.nextModal().has_value());
}
```

- [ ] **Step 2: Push red warning tests and confirm GitHub failure**

Commit `test: define configurable tile warning behavior`, push, and record the
expected native failure.

- [ ] **Step 3: Implement bounded state for all 26 logical tiles**

Maintain one runtime record per `TileAddress`: safe, pending, active-unacknowledged,
or active-acknowledged plus the pending start time. Disabled warnings reset to
safe. Invalid parameters reset pending state but preserve an already-active
highlight only until the next evaluation confirms invalidity, at which point
the highlight clears without producing a modal.

Calculate priority as
`abs(value - threshold) / max(abs(threshold), 1.0f)`. Sort descending priority,
then DASH before TRACK, then lower slot index. `WarningModalData` carries address,
parameter, current native value, native threshold, and remaining count; the UI
converts values for display.

- [ ] **Step 4: Retire fixed threshold alarms from UI-facing behavior**

Keep `AlarmManager` only as a compatibility wrapper during the transition or
replace its use in `App`; no hard-coded temperature, oil, lambda, or battery
threshold may continue to style tiles once configurable warnings are active.
Update existing tests to assert the wrapper is no longer the dashboard warning
source.

- [ ] **Step 5: Push green warning engine and verify GitHub Actions**

Commit `feat: add persistent per-tile warning state machine`, push, and require
the full workflow to pass.

### Task 6: Explicit data source and safe CAN reconfiguration

**Files:**
- Modify: `src/telemetry/telemetry_manager.h`
- Modify: `src/telemetry/telemetry_manager.cpp`
- Modify: `src/can/can_driver.h`
- Modify: `src/can/can_driver.cpp`
- Modify: `src/app/app.h`
- Modify: `src/app/app.cpp`
- Modify: `platformio.ini`
- Test: `test/test_source_selection/test_main.cpp`
- Modify: `test/test_telemetry_manager/test_main.cpp`

**Interfaces:**
- Consumes: `AppConfig::data_source`, `CanSettings`.
- Produces: `TelemetryManager::selectSource(DataSource)`, `TelemetryManager::setDecoder(const EcuCanDecoder&)`, `App::applyCanSettings(const CanSettings&)`, and expanded CAN counters.

- [ ] **Step 1: Write failing no-fallback tests**

```cpp
void test_can_timeout_never_activates_demo() {
    EcuCanDecoder decoder(nullptr, 0U);
    TelemetryManager telemetry(decoder, 500U);
    telemetry.selectSource(DataSource::Can);
    telemetry.setCanInitialized(true, 0U);
    telemetry.update(501U);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(DataSource::None),
                            static_cast<uint8_t>(telemetry.state().source()));
    TEST_ASSERT_FALSE(telemetry.demoActive());
}

void test_demo_is_used_only_when_selected() {
    EcuCanDecoder decoder(nullptr, 0U);
    TelemetryManager telemetry(decoder, 500U);
    telemetry.selectSource(DataSource::Demo);
    telemetry.update(100U);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(DataSource::Demo),
                            static_cast<uint8_t>(telemetry.state().source()));
}
```

- [ ] **Step 2: Push red source-selection tests to GitHub**

Commit `test: require explicit CAN or DEMO selection`, push, and confirm the
native test failure on the old fallback constructor.

- [ ] **Step 3: Refactor telemetry selection**

Remove `demo_enabled` and automatic fallback. In DEMO mode, update and expose
`MockTelemetry` without requiring CAN. In CAN mode, expose CAN state only while
the selected decoder has fresh accepted data; otherwise expose a `None` state
and Waiting/Offline/InitFailed status.

- [ ] **Step 4: Make dashboard CAN receive-only and reconfigurable**

Set the TWAI general mode to `TWAI_MODE_LISTEN_ONLY`, preserve the zero-length TX
queue, add dropped/error counters from TWAI status where supported, and expose a
single stop/configure/start path. `App::applyCanSettings()` stops CAN, resets
telemetry freshness, selects the decoder supplied by the later profile registry,
starts the validated bitrate with the current safe empty decoder, and reports
failure without blocking settings or DEMO mode. The profile plan will replace
that empty decoder through the same reconfiguration path.

- [ ] **Step 5: Push green source behavior and verify GitHub Actions**

Commit `refactor: make dashboard sources explicit and CAN receive only`, push,
and require the complete workflow to pass.

### Task 7: Rebuild DASH/TRACK and remove DIAG

**Files:**
- Create: `src/ui/ui_theme.h`
- Create: `src/ui/tile_view.h`
- Create: `src/ui/tile_view.cpp`
- Create: `src/ui/shift_light_view.h`
- Create: `src/ui/shift_light_view.cpp`
- Modify: `src/ui/ui.h`
- Modify: `src/ui/ui.cpp`
- Modify: `src/app/app.cpp`

**Interfaces:**
- Consumes: `TileEngine` placements, `UnitPresenter`, `ShiftLightModel`, `AppConfig`, `VehicleState`.
- Produces: three-page `Ui`, reusable `TileView`, identical DASH/TRACK `ShiftLightView`, and tile tap events carrying `TileAddress`.

- [ ] **Step 1: Split reusable LVGL rendering from page construction**

`TileView` owns the LVGL panel, accent stripe, centered/non-centered label, value,
unit, and warning border. `Ui` owns fixed arrays of 14 DASH and 12 TRACK tile
views and creates/deletes no tiles during normal updates; it applies visibility
and geometry from `TileEngine`.

- [ ] **Step 2: Build the approved DASH page**

Use the geometry constants from Task 3. Render four left small tiles, two center
wide tiles with centered label/value, four center-small tiles, and four right
small tiles. Default RPM and gear remain ordinary configurable wide slots.
Attach `LV_EVENT_CLICKED` to every visible tile and forward its `TileAddress`.

- [ ] **Step 3: Build the approved TRACK page with the same shift strip**

Render four left small, four centered center-wide, and four right small tiles.
Instantiate `ShiftLightView` with the same dimensions and segment model used by
DASH. Feed both instances the same `AppConfig::shift` and current RPM on every
UI update.

- [ ] **Step 4: Replace navigation and delete DIAG paths**

Change `Page` to `{Dash, Track, Settings}`, create three buttons each covering
one third of 800 px, remove `diag_`, `createDiag`, diagnostic labels, DIAG event
values, and the DIAG startup log text. Keep CAN counters available to the later
SETTINGS view.

- [ ] **Step 5: Push firmware UI changes and verify on GitHub**

Commit `feat: rebuild configurable dash and track pages`, push, and require the
GitHub target firmware build and artifact upload to pass. This task has no local
LVGL execution and makes no visual-completion claim yet.

### Task 8: Tile editor, SETTINGS, warnings, and software brightness

**Files:**
- Create: `src/ui/tile_editor_view.h`
- Create: `src/ui/tile_editor_view.cpp`
- Create: `src/ui/settings_view.h`
- Create: `src/ui/settings_view.cpp`
- Create: `src/ui/warning_modal.h`
- Create: `src/ui/warning_modal.cpp`
- Modify: `src/ui/ui.h`
- Modify: `src/ui/ui.cpp`
- Modify: `src/board/board_display.h`
- Modify: `src/board/board_display.cpp`
- Modify: `src/app/app.h`
- Modify: `src/app/app.cpp`

**Interfaces:**
- Consumes: `TileEditorModel`, `ConfigRepository`, `TileWarningEngine`, `UnitPresenter`, `AppConfig`, CAN runtime status.
- Produces: interactive editor callbacks, hidden-slot restoration, settings callbacks, global warning modal, and `BoardDisplay::setSoftwareBrightness(uint8_t)`.

- [ ] **Step 1: Implement the staged tile editor view**

Create a modal with parameter dropdown, visibility switch, decimals 0-3,
warning switch, above/below selector, threshold, hysteresis, delay, Save, and
Cancel. Convert threshold and hysteresis from presentation to native units when
building the candidate. Save calls the repository transaction first; only a
successful result updates runtime configuration and repacks the page. On failure
keep the editor open and display `SAVE FAILED`.

- [ ] **Step 2: Implement scrollable SETTINGS sections**

Add Display, Data source and CAN, Shift lights, Units, Layouts, and System.
Layout lists expose all 14 DASH and 12 TRACK logical slots including hidden
ones, and open the shared editor. CAN status and counters are read-only.
Profile dropdown initially contains only the safe empty profile; later plans
populate it from `CanProfileRegistry`.

- [ ] **Step 3: Implement settings validation and persistence**

Brightness preview applies immediately while the slider moves, but Cancel or a
failed settings save restores the previous brightness. Reject shift settings
unless `start < red <= maximum`. Unit changes trigger presentation refresh only.
Reset Layouts restores approved DASH/TRACK defaults. Factory Reset erases only
the application namespace, loads defaults, reconfigures runtime source/CAN, and
returns to DASH.

- [ ] **Step 4: Implement non-interactive software dimming**

Create one full-screen black LVGL overlay on `lv_layer_top()` with opacity
mapped from 100% brightness to transparent and 20% to the darkest supported
value. Clear `LV_OBJ_FLAG_CLICKABLE` and `LV_OBJ_FLAG_SCROLLABLE` so touch passes
through. Keep the warning modal above page content while applying the same
global dimming level to the whole display.

- [ ] **Step 5: Implement warning modal and tile highlight**

Show `WARNING`, parameter name, live converted value/unit, converted configured
limit, remaining count, and an acknowledge button. Acknowledge calls the engine
and advances to its next modal. Independently set the related tile border red
while `isHighlighted(address)` is true, even after acknowledgement. Hidden tile
alarms still create the global modal.

- [ ] **Step 6: Push complete interactive UI and verify GitHub build**

Commit `feat: add tile editor settings and warning modal`, push, and require all
native tests, the firmware build, packaging checks, and artifact upload to pass.

### Task 9: Deterministic UI previews and documentation

**Files:**
- Create: `scripts/ui_preview/render_ui_preview.py`
- Create: `scripts/ui_preview/ui_contract.json`
- Create: `scripts/tests/test_ui_preview.py`
- Modify: `.github/workflows/build-firmware.yml`
- Modify: `README.md`
- Create: `docs/ui/dashboard-config-guide.md`

**Interfaces:**
- Consumes: approved fixed 800x480 geometry, default configuration, colors, and labels from the passing revision.
- Produces: `ui-preview-dash.png`, `ui-preview-track.png`, `ui-preview-settings.png`, `ui-preview-tile-editor.png`, and `ui-preview-warning.png` in the GitHub artifact.

- [ ] **Step 1: Write a failing preview artifact test**

```python
class PreviewContractTest(unittest.TestCase):
    def test_renderer_creates_all_800x480_views(self):
        with tempfile.TemporaryDirectory() as output:
            render_all(Path(output))
            for name in EXPECTED_PREVIEWS:
                image = Image.open(Path(output) / name)
                self.assertEqual((800, 480), image.size)
```

Add `Pillow==11.3.0` only to the CI Python install step used by
preview generation; it is not a firmware dependency.

- [ ] **Step 2: Push the red preview test**

Commit `test: define UI preview artifact contract`, push, and confirm the
`Test firmware packaging` or dedicated preview test step fails because the
renderer is absent.

- [ ] **Step 3: Implement the deterministic renderer**

Render the exact theme colors, margins, navigation, shift strip, tile groups,
centered wide values, representative DEMO values, settings sections, tile
editor, and warning modal. Put the mirrored numeric geometry and palette values
in `scripts/ui_preview/ui_contract.json`; the test parses the named constants in
`tile_layout.h` and `ui_theme.h` and fails when the JSON diverges. This preview
is a review aid, not a claim that desktop Pillow renders LVGL pixels identically.

- [ ] **Step 4: Upload previews with the firmware artifact**

Run the renderer in GitHub Actions after native tests, copy its five PNG files
to `artifacts/`, and keep the existing firmware-binary checks unchanged.

- [ ] **Step 5: Document controls and limitations**

Document single-tap editing, Save/Cancel, restoring hidden slots, warning
acknowledgement/rearm, shared DASH/TRACK shift settings, explicit CAN/DEMO,
brightness range, unit behavior, and the difference between CI preview/build
evidence and physical hardware validation.

- [ ] **Step 6: Push green preview/docs change and verify final phase-1 CI**

Commit `docs: add dashboard controls and CI previews`, push, and require one
workflow run for the exact HEAD SHA to pass every step and upload firmware plus
all five PNG previews.

- [ ] **Step 7: Present the UI generated by the passing revision**

Download or open the five PNG files from that successful GitHub Actions
artifact and show DASH and TRACK directly to the user, with links/previews for
SETTINGS, tile editor, and warning modal. Report the workflow URL and exact
commit SHA. Do not describe the firmware as vehicle-validated.

## Phase-1 completion gate

Before starting the standalone ECU profile plan, verify from GitHub evidence:

- all native tests pass;
- Waveshare firmware compiles;
- packaging and binary checks pass;
- the artifact includes firmware binaries and five 800x480 preview images;
- the preview shows matching shift-light strips on DASH and TRACK;
- the branch contains no DIAG route or UI;
- CAN timeout does not activate DEMO;
- the worktree is clean after the final phase-1 commit.

Then write the source-audited profile implementation plan against the stable
`ParameterId`, `VehicleState`, CAN reconfiguration path, and decoder interfaces
established here. That plan introduces `CanProfileRegistry`.
