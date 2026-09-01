# BartzDash v0.2 ECUMaster + Configurable UI Design

## Goal

Extend the validated BartzDash v0.1 firmware for Waveshare ESP32-S3-Touch-LCD-5 (non-B, 800x480) with live ECUMaster EMU Black CAN input, a complete parameter registry for the standard EMU CAN stream, user-configurable dashboard tiles, warning configuration directly on the touchscreen, and persistent settings in NVS.

## Core design choice

Use a typed Parameter Registry between the ECU decoder and the UI.

The UI must never depend on raw CAN frame layouts or ECUMaster scaling. Every decoded signal is normalized into an engineering-unit parameter. Tiles, warnings, and settings refer to stable parameter IDs.

```text
CAN driver
   -> ECUMaster decoder
      -> Parameter Registry
         -> Alarm Manager
         -> UI / Settings
```

The user-facing name is `CAN`. The Espressif TWAI driver name may appear only inside the low-level CAN implementation.

## Hardware and platform

- Board: Waveshare ESP32-S3-Touch-LCD-5, non-B
- Resolution: 800x480
- MCU: ESP32-S3
- CAN TX: GPIO15
- CAN RX: GPIO16
- CAN transceiver: onboard TJA1051-class transceiver provided by the board
- Framework: PlatformIO + Arduino-ESP32
- Display abstraction: ESP32_Display_Panel
- UI: LVGL 8.4
- Storage: ESP32 NVS for persistent configuration

## CAN configuration

User-configurable from the display:

- Data source: MOCK / ECUMASTER
- Bitrate: 125 / 250 / 500 / 1000 kbit/s
- Base ID: default `0x600`
- Frame timeout: default 500 ms

Default ECUMaster profile:

- standard 11-bit CAN identifiers
- little-endian decoding
- base ID `0x600`
- nominal bitrate 1 Mbit/s

## ECUMaster parameter mapping

Version 0.2 shall support the full standard EMU Black stream represented by frames from base ID `0x600` through `0x607`.

The registry must include at least the following logical parameters when present in the standard stream:

### Engine / load

- RPM
- TPS
- MAP / boost pressure
- boost target
- injector pulse width
- ignition angle
- dwell

### Temperatures

- IAT
- CLT
- oil temperature
- ECU temperature
- EGT1
- EGT2

### Pressures

- barometric pressure
- oil pressure
- fuel pressure

### Lambda / fueling

- lambda
- lambda target
- lambda correction
- ethanol content
- fuel used

### Vehicle / drivetrain

- vehicle speed
- gear
- DBW position
- DBW target

### Electrical / analog

- battery voltage
- AIN1
- AIN2
- AIN3
- AIN4
- AIN5
- AIN6
- PWM1
- PWM2

### Traction / limiter / torque-management values

- TC differential RPM raw
- TC differential RPM filtered
- TC torque reduction
- pit limiter torque reduction

### Status and flags

Where provided by the stream, expose status/boolean parameters for:

- ECU error/status flags
- launch control
- anti-lag
- gear cut
- traction control
- pit limiter
- brake input
- fuel pump
- fan
- nitrous
- CAN switch/status bits

Exact byte offsets, signedness, scaling, and units must be implemented from the current official ECUMaster EMU Black CAN stream documentation and covered by decoder tests.

## Parameter Registry

### Stable ID

Every parameter has a compile-time stable `ParameterId` enum value. UI layout and NVS store IDs, not display strings.

### Metadata

Each registry entry contains:

```cpp
struct ParameterDescriptor {
    ParameterId id;
    const char* name;
    const char* short_name;
    const char* unit;
    uint8_t decimals;
    float display_min;
    float display_max;
    bool is_boolean;
};
```

### Runtime state

```cpp
struct ParameterValue {
    float value;
    bool valid;
    uint32_t updated_ms;
};
```

Boolean/status parameters use numeric 0/1 runtime representation but are rendered as OFF/ON or inactive/active by UI helpers.

### Registry API

The UI and alarm manager consume a stable API such as:

```cpp
const ParameterDescriptor& parameterDescriptor(ParameterId id);
const ParameterValue& parameterValue(ParameterId id);
void setParameterValue(ParameterId id, float value, uint32_t now_ms);
void invalidateAllCanParameters();
```

Raw CAN scaling must never appear in UI or warning code.

## Data sources

Two producers write into the same registry:

### MOCK

- retained for bench testing without an ECU
- generates realistic values
- marks data as valid
- clearly identifies source as MOCK in DIAG/SETTINGS

### ECUMASTER

- owns the real CAN decoder
- updates parameters only from valid received frames
- records per-frame receive timestamps

Switching source from the UI reinitializes the producer without rebooting where practical. If a runtime reinitialization cannot be made reliable, the UI may request a controlled restart after saving the new source.

## CAN timeout and stale data

Default CAN timeout is 500 ms.

If no valid ECUMaster stream frames are received within the configured timeout:

- CAN status changes to LOST/OFFLINE
- all ECUMaster-origin parameters are marked invalid
- UI continues running
- stale values are not presented as current measurements
- active alarms that depend on invalid data are suppressed and replaced by the CAN-loss condition

A dedicated CAN LOST warning can be enabled by default.

## Configurable dashboard tiles

### Tile model

Each configurable tile stores:

```cpp
struct TileConfig {
    ParameterId parameter;
    bool custom_label_enabled;
    char custom_label[16];
};
```

Formatting (unit and decimal count) defaults to the parameter descriptor. Future versions may expose per-tile formatting overrides, but v0.2 should avoid unnecessary complexity unless required by the layout.

### Editing flow

A long press on a configurable tile opens an edit screen.

The user can:

- choose the displayed parameter from the registry
- optionally set a short custom label
- open warning settings for that parameter
- save or cancel

Parameter selection should use a scrollable, touch-friendly list grouped by category (Engine, Temperature, Pressure, Lambda/Fuel, Vehicle, Analog, Status).

### Screens

Existing DASH and TRACK screens remain. Their secondary tiles become configurable. Core navigation controls and safety-critical fixed layout elements may remain fixed.

Default primary elements:

- DASH: RPM and gear remain fixed primary widgets
- TRACK: RPM and gear remain fixed primary widgets

Secondary telemetry tiles can be mapped to any compatible registry parameter.

## Warning system

### Warning modes

Each parameter supports one of:

- OFF
- LOW
- HIGH
- RANGE
- RPM_CURVE (only for parameters where this is enabled; required for oil pressure)

### Warning configuration

```cpp
enum class WarningMode : uint8_t {
    Off,
    Low,
    High,
    Range,
    RpmCurve,
};

struct WarningConfig {
    WarningMode mode;
    bool enabled;
    float warning_low;
    float warning_high;
    float critical_low;
    float critical_high;
    float hysteresis;
    uint16_t delay_ms;
    bool fullscreen_critical;
};
```

`RpmCurve` uses a separate fixed-size curve configuration.

### RPM-dependent oil-pressure warning

Required for oil pressure.

Use an editable fixed-point curve with up to 8 points:

```cpp
struct RpmWarningPoint {
    uint16_t rpm;
    float min_value;
};
```

The alarm manager linearly interpolates the minimum allowed oil pressure between points. Below the first RPM point and above the last point, clamp to the first/last threshold.

### Alarm states

Each configured warning evaluates to:

- NORMAL
- WARNING
- CRITICAL
- INVALID

Delay and hysteresis are applied before state transitions to reduce flicker and transient nuisance alarms.

### UI behavior

- WARNING: affected tile uses amber/yellow warning styling
- CRITICAL: affected tile uses red styling
- optionally show a full-screen critical overlay when `fullscreen_critical` is enabled
- INVALID: value renders as `---` or equivalent invalid-data indicator

Critical overlay must show parameter name, current value, unit, and threshold context.

## SETTINGS screen

Add a dedicated settings flow accessible from the touchscreen.

### CAN

- source MOCK / ECUMASTER
- bitrate
- base ID
- timeout
- live CAN status
- received frame counter
- lost/invalid frame counter where available

### DASH LAYOUT

- list configurable DASH tiles
- tap a tile slot to choose parameter
- long press from DASH provides direct access to the same editor

### TRACK LAYOUT

- equivalent configuration for TRACK secondary tiles

### WARNINGS

- parameter list
- warning enabled
- mode
- thresholds
- hysteresis
- delay
- fullscreen critical toggle
- oil-pressure RPM curve editor

### DISPLAY

v0.2 may retain current brightness/theme behavior if it already exists; no new display feature is required unless needed by settings navigation.

### SYSTEM

- firmware version
- free heap
- PSRAM
- factory reset of BartzDash configuration

## Persistence

Use ESP32 NVS.

Persist:

- selected data source
- CAN bitrate
- CAN base ID
- CAN timeout
- DASH tile mapping
- TRACK tile mapping
- custom labels
- warning configurations
- oil-pressure RPM warning curve

Configuration requires a schema version.

```cpp
constexpr uint16_t CONFIG_SCHEMA_VERSION = 2;
```

On missing or incompatible configuration, load safe defaults and save the current schema.

Factory reset removes BartzDash-owned NVS settings and restores defaults.

## Default warning presets

Defaults must be conservative and editable. Because vehicle-specific safe limits vary, defaults are initial UI values, not universal mechanical guarantees.

Recommended initial examples:

- CAN LOST: enabled
- CLT: HIGH warning 105 C, critical 115 C
- oil temp: HIGH warning 120 C, critical 135 C
- battery: LOW warning 11.8 V, critical 11.0 V
- oil pressure: RPM_CURVE enabled with an editable conservative starter curve

Fuel-pressure and lambda warnings should default OFF unless a vehicle-specific target context is available, because fixed generic thresholds can produce misleading alarms.

## Suggested code structure

Keep v0.1 responsibilities and add focused modules:

```text
src/
  app/
  board/
  can/
    can_driver.cpp
    can_driver.h
  ecu/
    ecumaster_decoder.cpp
    ecumaster_decoder.h
  telemetry/
    parameter_id.h
    parameter_registry.cpp
    parameter_registry.h
    mock_telemetry.cpp
    mock_telemetry.h
  alarms/
    alarm_manager.cpp
    alarm_manager.h
    warning_config.h
  settings/
    app_config.cpp
    app_config.h
    nvs_config_store.cpp
    nvs_config_store.h
  ui/
    ...existing UI...
    screen_settings.cpp
    screen_settings.h
    tile_editor.cpp
    tile_editor.h
    warning_editor.cpp
    warning_editor.h
```

Large existing `ui.cpp` logic should only be split where necessary for v0.2 features; avoid unrelated refactoring.

## Testing strategy

### Native/unit tests

Add tests for logic that can run without ESP32 hardware:

1. Parameter Registry
   - descriptor lookup
   - value update
   - invalidation

2. ECUMaster decoder
   - one or more fixture frames for every supported ID `0x600` through `0x607`
   - signed values
   - endian correctness
   - scale correctness
   - status flags
   - configurable base-ID offset

3. Alarm Manager
   - LOW
   - HIGH
   - RANGE
   - delay
   - hysteresis
   - invalid input
   - RPM-curve interpolation and clamping

4. Configuration
   - defaults
   - schema version handling where host-testable
   - tile mapping serialization representation where host-testable

### Firmware compile test

GitHub Actions must continue to build target firmware successfully and publish binaries.

### Hardware acceptance

On the target display:

1. v0.2 boots and preserves v0.1 LCD/touch behavior
2. MOCK mode works
3. source can be changed to ECUMASTER from SETTINGS
4. live CAN status appears
5. real EMU Black data updates mapped parameters
6. unplugging CAN causes CAN LOST and invalidates stale values
7. long-pressing a tile opens its editor
8. changing a tile parameter immediately changes displayed telemetry
9. configuration survives power cycle
10. warning threshold can be edited on-screen
11. warning/critical tile styling triggers correctly
12. oil-pressure RPM curve can be edited and evaluated
13. factory reset restores defaults

## GitHub Actions / artifacts

Continue the existing CI pattern.

A v0.2 build is not considered complete until:

- native tests pass
- target firmware build passes
- artifact upload passes
- firmware binaries are available

Hardware success must be reported separately after testing on the real board.

## Definition of done

BartzDash v0.2 is complete when:

- ECUMaster standard stream `0x600-0x607` is decoded through the Parameter Registry
- user-facing terminology is CAN, not TWAI
- secondary DASH/TRACK tiles are configurable from the touchscreen
- warning configuration is editable from the touchscreen
- warning settings persist in NVS
- CAN loss invalidates stale data
- oil-pressure RPM-dependent warning is implemented
- unit tests and GitHub Actions are green
- the user validates live ECUMaster operation on the physical display
