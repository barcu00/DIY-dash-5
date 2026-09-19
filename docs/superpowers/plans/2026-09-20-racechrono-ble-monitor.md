# RaceChrono BLE Monitor Integration Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add an optional RaceChrono DIY BLE Monitor overlay whose lap, delta, GPS, and calculated values can be selected in ordinary dashboard tiles while CAN or DEMO remains the engine-data source.

**Architecture:** A transport-independent RaceChrono protocol/session layer consumes bounded BLE events and publishes values to a dedicated `RaceChronoTelemetry` store. `CompositeTelemetryView` routes existing parameter IDs to `VehicleState` and appended RaceChrono IDs to that store, so tiles and tile warnings can use both sources without adding a third `DataSource`. The ESP32-S3-only NimBLE adapter owns GATT and advertising, while LVGL consumes immutable runtime snapshots through the normal 50 Hz render path.

**Tech Stack:** C++17, Arduino ESP32-S3 3.1.1, PlatformIO 6.1.18, NimBLE-Arduino 2.5.1, LVGL 8.4.0, Unity native tests, GitHub Actions.

**Spec:** `docs/superpowers/specs/2026-09-20-racechrono-ble-monitor-design.md`

## Global Constraints

- Keep `DataSource` limited to `Can` and `Demo`; RaceChrono is a supplemental overlay.
- Pin `h2zero/NimBLE-Arduino` to exactly `2.5.1` in the `waveshare_5` environment.
- Advertise device name `DIY DASH RC` and service UUID `0x1FF8`.
- Expose configuration characteristic `0x0005` as `INDICATE | WRITE` and values characteristic `0x0006` as `WRITE_WITHOUT_RESPONSE`.
- BLE callbacks may only copy bounded events into the fixed queue; they must never call LVGL or NVS.
- Process at most eight BLE events per `App::loop()` iteration before returning to CAN, shift-light, UI, and display service.
- Keep dashboard rendering at the existing 50 Hz schedule and keep shift-light scheduling independent.
- Request `Update all` every 2000 ms; enter `NO DATA` after 2500 ms without a valid value packet; invalidate a channel after 5000 ms without refresh.
- Append new `ParameterId` values after every existing value and before `Count`; do not renumber existing parameters.
- Change configuration schema from 8 to 9 and migrate schema 8 without losing any field or tile bank.
- Keep every new UI label in English.
- Match the approved v4 RaceChrono screen at 800 × 480: one continuous panel, fixed rows, no four-card status row, and no background dashboard rendering.

## Review Focus

- Event-queue overflow: drop the newest event, increment a drop counter, and preserve all queued events without blocking the producer.
- `millis()` wraparound: 2000/2500/5000 ms deadlines must still work through unsigned 32-bit rollover.
- Partial configuration failure: one equation exception or retry exhaustion marks only that channel `ERROR` and configuration continues.
- Disable or restart during configuration: clear pending protocol work, disconnect cleanly, invalidate the overlay, and leave CAN/DEMO untouched.
- Schema-8 migration: preserve both current tile arrays, all ten alternate tile banks, active layouts, units, CAN settings, RPM settings, and warning sound.

---

### Task 1: RaceChrono parameter IDs, catalog, units, and picker metadata

**Files:**
- Create: `src/racechrono/racechrono_channel_catalog.h`
- Create: `src/racechrono/racechrono_channel_catalog.cpp`
- Create: `test/test_racechrono_channel_catalog/test_main.cpp`
- Modify: `src/telemetry/parameter_id.h`
- Modify: `src/telemetry/parameter_registry.cpp`
- Modify: `src/ui/unit_presenter.cpp`
- Modify: `src/ui/editor_value_model.h`
- Modify: `src/ui/editor_value_model.cpp`
- Modify: `src/ui/parameter_options.cpp`
- Modify: `src/ui/tile_refresh_policy.cpp`
- Modify: `test/test_parameter_registry/test_main.cpp`
- Modify: `test/test_parameter_options/test_main.cpp`
- Modify: `test/test_editor_value_model/test_main.cpp`
- Modify: `test/test_unit_presenter/test_main.cpp`
- Modify: `platformio.ini`

**Interfaces:**
- Produces: `bool isRaceChronoParameter(ParameterId id)`.
- Produces: `const RaceChronoChannelDescriptor* raceChronoChannel(ParameterId id)`.
- Produces: `const RaceChronoChannelDescriptor* raceChronoChannelByMonitorId(uint8_t monitor_id)`.
- Produces: `std::size_t raceChronoChannelCount()` and `const RaceChronoChannelDescriptor& raceChronoChannelAt(std::size_t index)`.
- Produces: `ParameterCategory::RaceChrono` for every appended RaceChrono parameter.
- Consumes: existing `ParameterDescriptor`, `NativeUnit`, `ParameterOptions`, and `UnitPresenter` conventions.

- [ ] **Step 1: Write catalog and registry tests that pin stable IDs, monitor IDs, equations, units, and categories**

Append these IDs in this exact order after `CruiseControlActive`: `RcLapNumber`, `RcLapTime`, `RcLapDistance`, `RcPreviousLapNumber`, `RcPreviousLapTime`, `RcBestLapNumber`, `RcBestLapTime`, `RcComparisonLapNumber`, `RcComparisonLapTime`, `RcDeltaLapTime`, `RcLapTimeGain`, `RcSectorTime`, `RcSectorDistance`, `RcTotalRaceTime`, `RcTotalRaceDistance`, `RcElapsedTime`, `RcGpsSpeed`, `RcAltitude`, `RcBearing`, `RcLatitude`, `RcLongitude`, `RcSatellites`, `RcFixType`, `RcAccuracy`, `RcCoordinatePrecision`, `RcAltitudePrecision`, `RcThreeDPrecision`, `RcDeviceUpdateRate`, `RcDeltaSpeed`, `RcLateralAcceleration`, `RcLongitudinalAcceleration`, `RcCombinedAcceleration`, `RcLeanAngle`.

Use a descriptor with explicit decoding metadata:

```cpp
enum class RaceChronoValueEncoding : uint8_t {
    ScaledSignedInt32,
    CoordinateDegreesTimes6000000,
};

struct RaceChronoChannelDescriptor {
    ParameterId parameter;
    uint8_t monitor_id;
    const char* equation;
    float raw_to_native;
    RaceChronoValueEncoding encoding;
    uint32_t stale_ms;
};
```

Pin the catalog to this mapping; `raw_to_native` is the inverse of the equation multiplier:

| ID | Equation | Scale |
|---|---|---:|
| 1 | `channel(device(lap), lap_number)` | 1 |
| 2 | `channel(device(lap), lap_time) * 1000` | 0.001 s |
| 3 | `channel(device(lap), lap_distance) * 10` | 0.1 m |
| 4 | `channel(device(lap), previous_lap_number)` | 1 |
| 5 | `channel(device(lap), previous_lap_time) * 1000` | 0.001 s |
| 6 | `channel(device(lap), best_lap_number)` | 1 |
| 7 | `channel(device(lap), best_lap_time) * 1000` | 0.001 s |
| 8 | `channel(device(lap), comparison_lap_number)` | 1 |
| 9 | `channel(device(lap), comparison_lap_time) * 1000` | 0.001 s |
| 10 | `channel(device(lap), delta_lap_time) * 1000` | 0.001 s |
| 11 | `channel(device(lap), lap_time_gain) * 1000` | 0.001 s |
| 12 | `channel(device(lap), sector_time) * 1000` | 0.001 s |
| 13 | `channel(device(lap), sector_distance) * 10` | 0.1 m |
| 14 | `channel(device(lap), total_race_time) * 1000` | 0.001 s |
| 15 | `channel(device(lap), total_race_distance) * 10` | 0.1 m |
| 16 | `channel(device(gps), elapsed_time) * 1000` | 0.001 s |
| 17 | `channel(device(gps), speed) * 360` | 0.01 km/h |
| 18 | `channel(device(gps), altitude) * 10` | 0.1 m |
| 19 | `channel(device(gps), bearing) * 100` | 0.01 deg |
| 20 | `channel(device(gps), latitude)` | degrees × 6,000,000 |
| 21 | `channel(device(gps), longitude)` | degrees × 6,000,000 |
| 22 | `channel(device(gps), satellites)` | 1 |
| 23 | `channel(device(gps), fix_type)` | 1 |
| 24 | `channel(device(gps), accuracy) * 100` | 0.01 m |
| 25 | `channel(device(gps), coordinate_precision) * 100` | 0.01 m |
| 26 | `channel(device(gps), altitude_precision) * 100` | 0.01 m |
| 27 | `channel(device(gps), 3d_precision) * 100` | 0.01 m |
| 28 | `channel(device(gps), device_update_rate) * 10` | 0.1 Hz |
| 29 | `channel(device(calc), delta_speed) * 360` | 0.01 km/h |
| 30 | `channel(device(calc), lateral_acc) * 1000` | 0.001 g |
| 31 | `channel(device(calc), longitudinal_acc) * 1000` | 0.001 g |
| 32 | `channel(device(calc), combined_acc) * 1000` | 0.001 g |
| 33 | `channel(device(calc), lean_angle) * 100` | 0.01 deg |

Tests must assert 33 unique, nonzero Monitor IDs, exact equations for GPS speed and calculated delta speed, coordinate encoding for latitude/longitude, 5000 ms staleness, and `THEORETICAL / REFERENCE` as the short name for `RcComparisonLapTime`.

- [ ] **Step 2: Run the focused tests and verify they fail before the catalog exists**

Run:

```powershell
pio test -e native -f test_racechrono_channel_catalog
pio test -e native -f test_parameter_registry
pio test -e native -f test_parameter_options
pio test -e native -f test_editor_value_model
pio test -e native -f test_unit_presenter
```

Expected: compilation fails on missing RaceChrono IDs, descriptor types, and `ParameterCategory::RaceChrono`.

- [ ] **Step 3: Implement the catalog and append presentation metadata**

Add `NativeUnit::Seconds`, `Meters`, `Hertz`, `GForce`, and `CoordinateDegrees`. Add English long/short labels and defaults: times 3 decimals, coordinates 5 decimals, precision/speed/angle 2 decimals, acceleration 2 decimals, and counters 0 decimals. Extend valid tile decimals from 0–3 to 0–5 and expose six decimal choices in the tile editor. Return 50 ms from `tileRefreshIntervalMs()` for live delta/speed/acceleration and 100 ms for the remaining RaceChrono values.

Update `ParameterOptions::build()` so every RaceChrono parameter is appended after the selected CAN/DEMO engine-capability list and is never disabled by a CAN profile. Preserve the current unavailable engine parameter as the first row when required; never duplicate a RaceChrono current selection.

- [ ] **Step 4: Run catalog and presentation tests**

Run the five focused commands from Step 2.

Expected: all pass, including five-decimal coordinates and the unchanged BMW MS43 engine-option count plus 33 RaceChrono options.

- [ ] **Step 5: Commit the catalog**

```powershell
git add platformio.ini src/racechrono src/telemetry src/ui test/test_racechrono_channel_catalog test/test_parameter_registry test/test_parameter_options test/test_editor_value_model test/test_unit_presenter
git commit -m "feat: define RaceChrono monitor channel catalog"
```

---

### Task 2: Pure Monitor API framing and value decoding

**Files:**
- Create: `src/racechrono/racechrono_protocol.h`
- Create: `src/racechrono/racechrono_protocol.cpp`
- Create: `test/test_racechrono_protocol/test_main.cpp`
- Modify: `platformio.ini`

**Interfaces:**
- Consumes: `RaceChronoChannelDescriptor` from Task 1.
- Produces: `RaceChronoPacket RaceChronoProtocol::removeAll()`.
- Produces: `RaceChronoPacket RaceChronoProtocol::updateAll()`.
- Produces: `std::size_t RaceChronoProtocol::fragmentCount(const char* equation)`.
- Produces: `RaceChronoPacket RaceChronoProtocol::addFragment(uint8_t monitor_id, const char* equation, std::size_t fragment_index)`.
- Produces: `RaceChronoConfigResult RaceChronoProtocol::decodeConfigResult(const uint8_t* data, std::size_t size)`.
- Produces: `RaceChronoValueBatch RaceChronoProtocol::decodeValues(const uint8_t* data, std::size_t size)`.

- [ ] **Step 1: Write failing codec tests**

Define fixed-size outputs with no dynamic allocation:

```cpp
struct RaceChronoPacket {
    std::array<uint8_t, 20> bytes{};
    uint8_t size = 0U;
};

struct RaceChronoDecodedValue {
    uint8_t monitor_id = 0U;
    int32_t raw = 0;
};

enum class RaceChronoDecodeError : uint8_t {
    None,
    Empty,
    InvalidLength,
    TooManyValues,
};

struct RaceChronoValueBatch {
    std::array<RaceChronoDecodedValue, 4> values{};
    uint8_t count = 0U;
    RaceChronoDecodeError error = RaceChronoDecodeError::None;
};
```

Tests must pin: remove-all byte `{0}`, update-all byte `{4}`, a 17-byte payload limit, fragment sequence beginning at zero, final command `3`, preceding command `2`, four values per 20-byte write, big-endian positive `0x000004D2 == 1234`, big-endian negative `0xFFFFFEC0 == -320`, atomic rejection of 19-byte input, and decoding of config results `Success`, `PayloadOutOfSequence`, and `EquationException` with exception type/position/length.

- [ ] **Step 2: Run the codec test and verify RED**

```powershell
pio test -e native -f test_racechrono_protocol
```

Expected: compilation fails because `racechrono_protocol.h` is absent.

- [ ] **Step 3: Implement the codec with byte-wise shifts**

Use explicit shifts rather than pointer casts:

```cpp
const uint32_t encoded =
    (static_cast<uint32_t>(data[offset + 1U]) << 24U) |
    (static_cast<uint32_t>(data[offset + 2U]) << 16U) |
    (static_cast<uint32_t>(data[offset + 3U]) << 8U) |
    static_cast<uint32_t>(data[offset + 4U]);
const int32_t raw = static_cast<int32_t>(encoded);
```

Reject the whole values write before decoding if `size == 0`, `size > 20`, or `size % 5 != 0`. `addFragment()` returns `size == 0` for an invalid index or null equation.

- [ ] **Step 4: Run the codec test and full native suite**

```powershell
pio test -e native -f test_racechrono_protocol
pio test -e native
```

Expected: all tests pass.

- [ ] **Step 5: Commit the codec**

```powershell
git add platformio.ini src/racechrono/racechrono_protocol.* test/test_racechrono_protocol
git commit -m "feat: encode and decode RaceChrono monitor protocol"
```

---

### Task 3: Fixed event queue, telemetry store, and session state machine

**Files:**
- Create: `src/racechrono/racechrono_event_queue.h`
- Create: `src/racechrono/racechrono_telemetry.h`
- Create: `src/racechrono/racechrono_telemetry.cpp`
- Create: `src/racechrono/racechrono_session.h`
- Create: `src/racechrono/racechrono_session.cpp`
- Create: `test/test_racechrono_event_queue/test_main.cpp`
- Create: `test/test_racechrono_telemetry/test_main.cpp`
- Create: `test/test_racechrono_session/test_main.cpp`
- Modify: `platformio.ini`

**Interfaces:**
- Consumes: protocol packets/results from Task 2 and catalog metadata from Task 1.
- Produces: `RaceChronoEventQueue<32>::pushFromProducer`, `popFromConsumer`, `droppedCount`, and `clear`.
- Produces: `RaceChronoTelemetry::acceptRaw`, `acceptBatch`, `updateStale`, `invalidateAll`, `get`, `channelState`, and `snapshotStatus`.
- Produces: `RaceChronoSession::setEnabled`, `onEvent`, `update`, and `takeAction`.
- Produces: connection states `Disabled`, `Advertising`, `Connected`, `Configuring`, `Active`, `NoData`, and `Error`.

- [ ] **Step 1: Write failing queue and telemetry tests**

Use a single-producer/single-consumer ring with atomic head/tail indices and capacity 32. Fill all usable slots, verify FIFO order, then push once more and assert the newest event is rejected, `droppedCount()` increases, and previously queued payloads remain byte-identical.

For telemetry, feed Monitor ID 10 with raw `-320` at `now_ms=1000` and assert `RcDeltaLapTime == -0.320f`. Feed latitude raw `313200000` and assert `52.2` degrees. Feed sentinel `0x7FFFFFFF` and assert invalid. Verify unknown IDs increment `unknown_monitor_ids` without changing any signal. Verify a malformed batch increments `malformed_packets` and publishes nothing.

- [ ] **Step 2: Write failing session tests for the complete state graph**

Exercise these exact transitions:

```text
setEnabled(true) -> Advertising
Connected -> Connected
IndicationsSubscribed -> Configuring
RemoveAllConfirmed -> first Add fragment
AddComplete + Success -> next channel
last channel Success -> UpdateAll -> Active after first valid batch
2501 ms without valid packet -> NoData
new valid batch -> Active
Disconnected while enabled -> Advertising
setEnabled(false) -> Disabled + Disconnect/StopAdvertising actions
```

Inject an equation exception for channel 2 and assert channel 2 is `Error`, channel 3 is still configured, and global state is not `Error`. Inject out-of-sequence three times and assert only that channel becomes `Error`. Test a clock start near `UINT32_MAX - 1000U`, advance through rollover, and verify the 2000/2500/5000 ms comparisons remain correct.

- [ ] **Step 3: Run the three focused suites and verify RED**

```powershell
pio test -e native -f test_racechrono_event_queue
pio test -e native -f test_racechrono_telemetry
pio test -e native -f test_racechrono_session
```

Expected: compilation fails on the new interfaces.

- [ ] **Step 4: Implement the queue, store, and state machine**

Represent transport input and output without NimBLE types:

```cpp
enum class RaceChronoEventType : uint8_t {
    Connected,
    Disconnected,
    IndicationsSubscribed,
    IndicationConfirmed,
    ConfigWrite,
    ValuesWrite,
};

struct RaceChronoEvent {
    RaceChronoEventType type = RaceChronoEventType::Disconnected;
    std::array<uint8_t, 20> bytes{};
    uint8_t size = 0U;
};

enum class RaceChronoActionType : uint8_t {
    StartAdvertising,
    StopAdvertising,
    Disconnect,
    Indicate,
};
```

Store exactly one `SignalValue` and one `RaceChronoChannelState` per catalog entry. Use unsigned subtraction for all deadlines. Request `UpdateAll` at 2000 ms cadence only in `Active` or `NoData`; never enqueue a second update while an indication is outstanding.

- [ ] **Step 5: Run focused and full native tests**

```powershell
pio test -e native -f test_racechrono_event_queue
pio test -e native -f test_racechrono_telemetry
pio test -e native -f test_racechrono_session
pio test -e native
```

Expected: all pass.

- [ ] **Step 6: Commit the session core**

```powershell
git add platformio.ini src/racechrono test/test_racechrono_event_queue test/test_racechrono_telemetry test/test_racechrono_session
git commit -m "feat: add RaceChrono telemetry session core"
```

---

### Task 4: Composite telemetry in tiles, warnings, and editor preview

**Files:**
- Create: `src/telemetry/composite_telemetry_view.h`
- Create: `src/telemetry/composite_telemetry_view.cpp`
- Create: `test/test_composite_telemetry_view/test_main.cpp`
- Modify: `src/alarms/tile_warning_engine.h`
- Modify: `src/alarms/tile_warning_engine.cpp`
- Modify: `src/ui/tile_view.h`
- Modify: `src/ui/tile_view.cpp`
- Modify: `src/ui/ui.h`
- Modify: `src/ui/ui.cpp`
- Modify: `src/ui/ui_tile_editor.cpp`
- Modify: `test/test_tile_warning_engine/test_main.cpp`
- Modify: `test/test_parameter_options/test_main.cpp`
- Modify: `platformio.ini`

**Interfaces:**
- Consumes: `VehicleState` and `RaceChronoTelemetry` from earlier tasks.
- Produces: `CompositeTelemetryView(const VehicleState&, const RaceChronoTelemetry&)`.
- Produces: `const SignalValue& CompositeTelemetryView::get(ParameterId id) const`.
- Produces: `bool CompositeTelemetryView::availableFromSupplement(ParameterId id) const`.
- Produces: `const VehicleState& CompositeTelemetryView::engineState() const` and `const RaceChronoTelemetry& CompositeTelemetryView::raceChronoState() const` for safe UI snapshots.
- Changes: `TileWarningEngine::evaluate` and `TileView::update` consume `CompositeTelemetryView`.

- [ ] **Step 1: Write failing routing and warning tests**

```cpp
VehicleState engine;
engine.reset(DataSource::Can);
engine.set(ParameterId::Rpm, 6840.0f, 100U);
RaceChronoTelemetry racechrono;
racechrono.acceptRaw(10U, -240, 100U);
CompositeTelemetryView view(engine, racechrono);

TEST_ASSERT_FLOAT_WITHIN(0.001f, 6840.0f,
                         view.get(ParameterId::Rpm).value);
TEST_ASSERT_FLOAT_WITHIN(0.001f, -0.240f,
                         view.get(ParameterId::RcDeltaLapTime).value);
```

Add a tile-warning test using `RcGpsSpeed` above a configured threshold, then invalidate only the RaceChrono store and assert the warning returns safe while the engine RPM signal remains valid. Add an option-list test proving all 33 RaceChrono IDs appear for both DEMO and BMW MS43 CAN.

- [ ] **Step 2: Run focused tests and verify RED**

```powershell
pio test -e native -f test_composite_telemetry_view
pio test -e native -f test_tile_warning_engine
pio test -e native -f test_parameter_options
```

Expected: compilation fails because consumers still require `VehicleState`.

- [ ] **Step 3: Implement composite routing without changing RPM/shift-light input**

`CompositeTelemetryView::get()` routes only IDs for which `isRaceChronoParameter()` is true. All other IDs call `VehicleState::get()`. Keep `Ui::updateShiftLight(const VehicleState&, ...)` unchanged so BLE can never influence RPM or shift light.

Change `Ui::update()` to receive `const CompositeTelemetryView&`, use it for tile/warning rendering, and retain separate copies from `view.engineState()` and `view.raceChronoState()` for the tile editor and safe first-frame preparation. Replace direct `capabilities_.supports(id)` checks with:

```cpp
const bool supported = isRaceChronoParameter(id) ||
                       capabilities_.supports(id);
```

- [ ] **Step 4: Run focused tests, then the full native suite**

```powershell
pio test -e native -f test_composite_telemetry_view
pio test -e native -f test_tile_warning_engine
pio test -e native -f test_parameter_options
pio test -e native
```

Expected: all pass; existing engine warning tests remain unchanged.

- [ ] **Step 5: Commit composite telemetry support**

```powershell
git add platformio.ini src/telemetry/composite_telemetry_view.* src/alarms src/ui test/test_composite_telemetry_view test/test_tile_warning_engine test/test_parameter_options
git commit -m "feat: expose RaceChrono data to tiles and warnings"
```

---

### Task 5: Schema-9 settings and lossless schema-8 migration

**Files:**
- Modify: `src/settings/app_config.h`
- Modify: `src/settings/app_config.cpp`
- Modify: `src/settings/config_repository.cpp`
- Modify: `test/test_app_config/test_main.cpp`
- Modify: `test/test_config_repository/test_main.cpp`

**Interfaces:**
- Produces: `struct RaceChronoSettings { bool enabled = false; };`.
- Produces: `AppConfig::racechrono` and `AppConfig::kSchemaVersion == 9`.
- Consumes: existing raw-blob migration conventions in `ConfigRepository`.

- [ ] **Step 1: Add failing default, validation, round-trip, and migration tests**

Freeze the exact version-8 layout before changing `AppConfig`:

```cpp
struct StoredAppConfigV8 {
    uint32_t schema_version = 8U;
    DataSource data_source = DataSource::Demo;
    uint8_t brightness_percent = 100U;
    CanSettings can{};
    ShiftLightConfig shift{};
    UnitSettings units{};
    std::array<TileConfig, 14> dash_tiles{};
    std::array<TileConfig, 12> track_tiles{};
    DashboardLayout dash_layout = DashboardLayout::ClassicDash;
    DashboardLayout track_layout = DashboardLayout::ClassicTrack;
    uint16_t rpm_scale_max = 10000U;
    std::array<AppConfig::TileBank, 5> dash_alternate_tiles{};
    std::array<AppConfig::TileBank, 5> track_alternate_tiles{};
    bool warning_sound_enabled = true;
};
```

Set distinct values in every scalar and every bank, migrate, and compare each field. Assert `racechrono.enabled == false`, the backend is rewritten to `sizeof(AppConfig)`, a failed rewrite returns `MigrationWriteFailed` while retaining migrated runtime data, and enabling RaceChrono round-trips in schema 9.

- [ ] **Step 2: Run settings tests and verify RED**

```powershell
pio test -e native -f test_app_config
pio test -e native -f test_config_repository
```

Expected: failures on schema version and missing settings.

- [ ] **Step 3: Implement version-8 migration and validation**

Copy version-8 fields explicitly into `AppConfig::defaults()`, set `racechrono.enabled = false`, validate, then rewrite. Do not use `memcpy` between schema structs.

- [ ] **Step 4: Run settings tests and full native suite**

```powershell
pio test -e native -f test_app_config
pio test -e native -f test_config_repository
pio test -e native
```

Expected: all pass.

- [ ] **Step 5: Commit schema 9**

```powershell
git add src/settings test/test_app_config test/test_config_repository
git commit -m "feat: persist RaceChrono enable setting"
```

---

### Task 6: RaceChrono settings model and approved v4 LVGL screen

**Files:**
- Create: `src/ui/racechrono_settings_model.h`
- Create: `src/ui/racechrono_settings_model.cpp`
- Create: `src/ui/ui_racechrono.cpp`
- Create: `test/test_racechrono_settings_model/test_main.cpp`
- Modify: `src/ui/settings_flow_model.h`
- Modify: `src/ui/ui.h`
- Modify: `src/ui/ui.cpp`
- Modify: `src/ui/settings_actions.h`
- Modify: `src/ui/ui_tile_editor.cpp`
- Modify: `design/production-preview/render.cpp`
- Modify: `design/production-preview/transitions.cpp`
- Modify: `.github/workflows/production-view-previews.yml`
- Modify: `scripts/ui_preview/ui_contract.json`
- Modify: `scripts/ui_preview/render_ui_preview.py`
- Modify: `platformio.ini`

**Interfaces:**
- Consumes: `RaceChronoRuntimeStatus`, catalog/channel states, and `AppConfig::racechrono`.
- Produces: `RaceChronoSettingsModel::setFilter`, `setChannelCount`, `nextPage`, `previousPage`, `firstIndex`, and `pageCount` with six rows per page.
- Produces: `SettingsCategory::RaceChrono`.
- Produces: `Ui::createRaceChronoSettings`, `Ui::refreshRaceChronoSettings`, and `Ui::takeRaceChronoRestart()`.

- [ ] **Step 1: Write failing paging and filter-model tests**

For 33 channels assert six pages with first indices `0, 6, 12, 18, 24, 30`; the last page exposes three entries. For filter states `All`, `Active`, `NoData`, and `Error`, assert stable catalog ordering and that changing the filter resets the page to zero. Assert next/previous clamp at the ends.

- [ ] **Step 2: Run the model test and verify RED**

```powershell
pio test -e native -f test_racechrono_settings_model
```

Expected: compilation fails because the model is absent.

- [ ] **Step 3: Implement the pure model, then rerun its test**

```powershell
pio test -e native -f test_racechrono_settings_model
```

Expected: pass.

- [ ] **Step 4: Add the `RACECHRONO` entry and full-screen two-tab UI**

Place a full-width `RACECHRONO` button at `x=24, y=246, w=714, h=56` on the existing `DATA & CAN` panel. The new screen uses the same safe `showSettings()` replacement and `lv_obj_del_async()` path as other settings screens.

Build the connection page as one bordered panel with four 42 px vertical rows, a two-column/two-row unboxed summary, and a final signal/action row. Build the channels page as one panel with one filter dropdown, six 36 px data rows, and a pager. Use `UiTheme::border()` gray for borders and `UiTheme::blue()` only for the selected tab and enabled switch. Clip long labels; never enable LVGL scrolling.

The enable switch updates the staged `AppConfig`, calls `commit_model_.markDirty(true)`, and is persisted/reconfigured through the existing save-on-exit path. `RESTART BLE` only sets a one-shot UI request flag; the App owns the restart. Tab/filter/page actions do not save configuration.

Extend the tile picker from four to five categories with widths that fit 800 px:

```cpp
constexpr int kCategoryX = 20;
constexpr int kCategoryGap = 8;
constexpr int kCategoryWidth = 144;
```

Display `RACECHRONO` as the fifth category. Keep RaceChrono entries selectable while disabled or disconnected.

- [ ] **Step 5: Extend production preview and transition checks**

Add preview modes that render `CONNECTION` in `Active` and `NoData`, `CHANNELS` with six rows, the last three-row page, and the RaceChrono picker category. Add transition checks for opening from `DATA & CAN`, switching tabs, returning to settings, and saving a RaceChrono tile. Assert the first framebuffer after each transition equals a directly rendered framebuffer.

Update the preview workflow compile command to include pure RaceChrono sources while excluding `racechrono_ble_transport.cpp`:

```bash
$(find src/racechrono -name '*.cpp' ! -name 'racechrono_ble_transport.cpp')
```

- [ ] **Step 6: Run native, preview-script, and host LVGL checks**

```powershell
pio test -e native -f test_racechrono_settings_model
python -m unittest discover -s scripts/tests -v
```

Run the same Linux LVGL compile/render commands from `.github/workflows/production-view-previews.yml` through GitHub Actions after pushing this commit. Expected: the workflow publishes connection, channels, and picker PNGs with no transition mismatch.

- [ ] **Step 7: Commit the approved UI**

```powershell
git add platformio.ini src/ui test/test_racechrono_settings_model design/production-preview scripts/ui_preview .github/workflows/production-view-previews.yml
git commit -m "feat: add RaceChrono connection and channel settings UI"
```

---

### Task 7: NimBLE peripheral transport and App integration

**Files:**
- Create: `src/racechrono/racechrono_ble_transport.h`
- Create: `src/racechrono/racechrono_ble_transport.cpp`
- Create: `src/racechrono/racechrono_runtime.h`
- Create: `src/racechrono/racechrono_runtime.cpp`
- Create: `test/test_racechrono_app_flow/test_main.cpp`
- Modify: `src/app/app.h`
- Modify: `src/app/app.cpp`
- Modify: `src/ui/ui.h`
- Modify: `platformio.ini`

**Interfaces:**
- Consumes: `RaceChronoEventQueue<32>`, `RaceChronoSession`, `RaceChronoTelemetry`, and UI restart request.
- Produces: `RaceChronoBleTransport::begin`, `startAdvertising`, `stopAdvertising`, `disconnect`, `indicate`, `restart`, and `pollEvent`.
- Produces: GATT callbacks that emit only fixed `RaceChronoEvent` values.
- Changes: `App` processes at most eight BLE events per loop and executes session actions outside callbacks.

- [ ] **Step 1: Write a failing App-flow test against a fake transport**

Extract orchestration into a native-testable `RaceChronoRuntime` owned by `App`:

```cpp
class RaceChronoTransport {
public:
    virtual ~RaceChronoTransport() = default;
    virtual bool startAdvertising() = 0;
    virtual void stopAdvertising() = 0;
    virtual void disconnect() = 0;
    virtual bool indicate(const uint8_t* data, std::size_t size) = 0;
};
```

The fake records actions and injects events. Test that enabling starts advertising, a complete handshake sends remove/add/update packets in order, only eight queued events are consumed by one service call, restart performs disconnect then advertising, disabling invalidates RaceChrono signals, and engine `VehicleState` remains byte-for-byte unchanged.

- [ ] **Step 2: Run the App-flow test and verify RED**

```powershell
pio test -e native -f test_racechrono_app_flow
```

Expected: compilation fails on the missing runtime/transport interface.

- [ ] **Step 3: Implement the native-testable runtime and pass the fake-transport test**

`RaceChronoRuntime::service(now_ms)` drains at most eight events, calls `session.update(now_ms)`, executes all currently available actions, and then calls `telemetry.updateStale(now_ms)`. No method in this class includes NimBLE or LVGL headers.

- [ ] **Step 4: Add the ESP32-S3 NimBLE adapter**

In `platformio.ini` add:

```ini
lib_deps =
    h2zero/NimBLE-Arduino@2.5.1
```

Preserve the existing libraries below that line. Initialize NimBLE once, create service `0x1FF8`, create `0x0005` and `0x0006` with the specified properties, attach callbacks, start the service, and advertise the service UUID/name. Characteristic write callbacks reject payloads over 20 bytes before queueing. Connection, disconnection, subscription, indication confirmation, configuration writes, and values writes become queue events.

- [ ] **Step 5: Wire runtime configuration, render snapshots, and restart**

In `App::begin()`, initialize the BLE adapter after display initialization and before the first UI snapshot. In `applyRuntimeConfig()`, call `racechrono_runtime_.setEnabled(config_.racechrono.enabled, now_ms)` without stopping or reinitializing CAN. In `App::loop()`, service RaceChrono immediately after CAN polling, then construct `CompositeTelemetryView` for warnings and UI. If `ui_.takeRaceChronoRestart()` is true, call the runtime restart method outside the board/LVGL lock.

- [ ] **Step 6: Run native tests and compile the production firmware**

```powershell
pio test -e native -f test_racechrono_app_flow
pio test -e native
pio run -e waveshare_5
```

Expected: all native tests pass and `.pio/build/waveshare_5/DIY-Dash-ESP32-S3-Touch-LCD-5-full.bin` exists.

- [ ] **Step 7: Commit hardware integration**

```powershell
git add platformio.ini src/racechrono src/app src/ui test/test_racechrono_app_flow
git commit -m "feat: connect RaceChrono monitor over BLE"
```

---

### Task 8: Documentation, GitHub CI, visual artifacts, and firmware handoff

**Files:**
- Create: `docs/racechrono/setup.md`
- Create: `docs/ui/screenshots/racechrono-connection.png`
- Create: `docs/ui/screenshots/racechrono-channels.png`
- Create: `docs/ui/screenshots/racechrono-picker.png`
- Modify: `README.md`
- Modify: `.github/workflows/build-firmware.yml`
- Modify: `docs/ui/dashboard-config-guide.md`

**Interfaces:**
- Consumes: completed firmware, production preview renderer, and current GitHub Actions workflow.
- Produces: user setup instructions, screenshots, CI artifact, and a testable full binary.

- [ ] **Step 1: Add a documentation contract test before editing docs**

Create `scripts/tests/test_racechrono_docs.py` that asserts `README.md` and `docs/racechrono/setup.md` contain: `DIY DASH RC`, the exact RaceChrono add-device path, `CONNECTION`, `CHANNELS`, `CAN / DEMO remains active`, `0x1FF8`, and the full-binary filename. It must also verify all three screenshot paths exist.

- [ ] **Step 2: Run the contract test and verify RED**

```powershell
python -m unittest scripts.tests.test_racechrono_docs -v
```

Expected: failure because setup documentation and screenshots are absent.

- [ ] **Step 3: Render approved production screenshots and write setup documentation**

Document the one-time phone setup exactly:

```text
RaceChrono Settings
  Add other device
  RaceChrono DIY
  Bluetooth LE
  DIY DASH RC
  Monitor
```

Explain that RaceChrono initiates the connection after session start; `ADVERTISING` before a session is normal; CAN/DEMO remains active; `THEORETICAL / REFERENCE` follows the comparison lap selected by RaceChrono; and unavailable channels show `---`.

- [ ] **Step 4: Make GitHub Actions verify dependency pinning and upload the full binary plus RaceChrono previews**

Keep existing native and firmware jobs. Add a shell assertion:

```bash
grep -F 'h2zero/NimBLE-Arduino@2.5.1' platformio.ini
```

Ensure the existing artifact collection includes `DIY-Dash-ESP32-S3-Touch-LCD-5-full.bin` and the newly rendered RaceChrono PNGs.

- [ ] **Step 5: Run all local verification before pushing**

```powershell
pio test -e native
python -m unittest discover -s scripts/tests -v
pio run -e waveshare_5
Get-FileHash '.pio\build\waveshare_5\DIY-Dash-ESP32-S3-Touch-LCD-5-full.bin' -Algorithm SHA256
```

Expected: every command succeeds and the SHA-256 hash is printed for the handoff.

- [ ] **Step 6: Commit docs and CI, then push the feature branch**

```powershell
git add README.md docs/racechrono docs/ui/screenshots docs/ui/dashboard-config-guide.md scripts/tests/test_racechrono_docs.py .github/workflows/build-firmware.yml
git commit -m "docs: publish RaceChrono setup and firmware checks"
git push origin dashboard-dev
```

- [ ] **Step 7: Verify GitHub Actions rather than treating the local build as final**

```powershell
gh run list --workflow build-firmware.yml --branch dashboard-dev --limit 1
gh run watch --exit-status
gh run list --workflow production-view-previews.yml --branch dashboard-dev --limit 1
```

Expected: `DIY Dash firmware build` and `Production LVGL view previews` complete successfully. Download the `DIY-Dash-firmware` artifact, verify it contains only the expected firmware package files and screenshots, and compare the full-binary SHA-256 with the artifact downloaded from GitHub.

- [ ] **Step 8: Perform hardware acceptance and record results**

Flash the GitHub-generated full binary to the Waveshare ESP32-S3 board. Record free heap before enabling BLE, while advertising, while active, and after ten disconnect/reconnect cycles. Verify no monotonic heap loss, CAN/DEMO remains smooth, shift light remains independent, warnings work for a RaceChrono tile, both settings tabs remain artifact-free, and the user can repeat the Android/iOS setup flow described in the spec.

- [ ] **Step 9: Commit the hardware validation record if the board test passes**

Create `docs/racechrono/hardware-validation.md` containing firmware SHA-256, phone OS/app version, connection/reconnect results, heap measurements, channel results, and observed packet rate, then commit:

```powershell
git add docs/racechrono/hardware-validation.md
git commit -m "test: record RaceChrono hardware validation"
git push origin dashboard-dev
```
