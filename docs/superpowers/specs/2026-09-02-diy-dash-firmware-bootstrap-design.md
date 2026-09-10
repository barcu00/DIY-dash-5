# DIY Dash Firmware Bootstrap Design

## Goal

Deliver a buildable, modular first production-oriented firmware baseline for the
Waveshare ESP32-S3-Touch-LCD-5 non-B board. The firmware must initialize the
real display, GT911 touch controller, OPI PSRAM, LVGL, and ESP32 TWAI peripheral;
render a readable 800x480 motorsport dashboard; isolate ECU protocol details from
the UI; and publish flashable binaries through GitHub Actions.

This milestone does not claim ECUMaster protocol support. It provides a tested,
table-driven decoder framework whose mappings remain empty until verified CAN
IDs, layouts, scaling, and timeouts are supplied from authoritative protocol
information.

## Existing Code and Scope

The current `main` branch already contains useful PlatformIO configuration, a
Waveshare board manifest, `ESP32_Display_Panel` display/touch initialization,
LVGL integration, mock telemetry, three basic screens, native tests, and a merge
script. Those elements will be retained and corrected rather than replaced.

The remote experimental feature branch contains broader settings and UI work.
Only small, independently justified ideas may be reused. Its hard-coded ECU
mapping is explicitly out of scope because this milestone must not assume a CAN
protocol.

The project remains at the repository root. Moving the existing PlatformIO
project under `firmware/dashboard/` would add path churn without improving the
firmware architecture.

## Target Hardware and Toolchain

- Waveshare ESP32-S3-Touch-LCD-5, non-B model
- ESP32-S3, 16 MB flash, 8 MB OPI PSRAM
- 800x480 RGB LCD supported by the board profile
- GT911 capacitive touch
- onboard TJA1051 CAN transceiver
- TWAI TX GPIO15 and RX GPIO16
- PlatformIO with Arduino framework
- pinned ESP32 platform and library revisions
- LVGL 8.4.x
- `ESP32_Display_Panel` plus its required expander dependencies
- C++17

The board manifest remains the source of flash size and memory-type settings.
Startup diagnostics report the detected flash, PSRAM, LCD dimensions, touch
availability, and CAN initialization result over USB Serial at 115200 baud.

## Architecture

```text
TWAI driver
    -> EcuCanDecoder
        -> VehicleState
            -> AlarmManager
            -> DashboardUi

DemoTelemetry ----------------> VehicleState
BoardDisplay -----------------> DashboardUi
App coordinates all modules
```

Suggested source layout:

```text
src/
  app/
  board/
  can/
  config/
  ecu/
  telemetry/
  alarms/
  ui/
```

Hardware-specific headers are not included by pure decoder, state, timeout, or
alarm logic. This keeps those components buildable in the native PlatformIO test
environment.

## VehicleState

`VehicleState` is the single snapshot consumed by the UI. It contains at least
RPM, MAP/boost, lambda, TPS, CLT, IAT, oil pressure, oil temperature, battery
voltage, CAN status, data-source status, and timestamps/validity needed to avoid
presenting stale values as current.

Every update is explicitly associated with one source:

- `Can` for values decoded from received frames;
- `Demo` for generated test values;
- `None` when no valid source is available.

The UI never receives raw CAN frames and never applies ECU-specific scaling.

## CAN and Decoder Framework

`CanDriver` owns the ESP32 TWAI lifecycle and non-blocking receive operation. It
uses GPIO15/GPIO16, a configurable bitrate, a bounded RX queue, accept-all
filtering for bring-up, and exposes status and counters without leaking TWAI
types into application logic.

`EcuCanDecoder` receives neutral `CanFrame` values. A protocol profile is a list
of signal definitions containing:

- CAN ID and standard/extended frame format;
- byte and optional bit offset;
- byte order;
- raw type and width;
- scale and additive offset;
- target `VehicleSignal`;
- engineering unit metadata;
- per-signal timeout.

The decoder validates DLC and bounds before reading. It supports integer raw
types needed for later profiles and applies the generic conversion
`engineering_value = raw_value * scale + offset`. An empty default profile
accepts no telemetry frames and therefore cannot generate invented vehicle
values. Future EMU Black, EMU Classic, or other ECU profiles plug into this
interface without changing `VehicleState` or UI code.

## CAN Timeout and DEMO Mode

The application distinguishes four visible CAN states: initialization failure,
waiting, online, and timed out/offline. Receipt of an arbitrary CAN frame is not
enough to mark telemetry online; at least one frame must be accepted by the
active decoder profile.

DEMO mode is controlled by a central compile-time/runtime configuration value.
When enabled, it activates only after the configured valid-CAN timeout. The
whole snapshot is then marked `Demo`, and the dashboard displays a prominent
`DEMO` source label. A valid decoded CAN update switches atomically back to CAN
data. When DEMO is disabled, missing data remains invalid and is rendered as
`---`. CAN and generated fields are never combined in one snapshot.

## Alarm Handling

Alarm thresholds live in one configuration header and do not appear as numeric
literals in UI code. The baseline provides:

- high CLT;
- high IAT;
- low oil pressure;
- lean lambda while load is above its configured threshold;
- low battery voltage.

The alarm manager consumes only valid `VehicleState` values. UI alarm treatment
uses amber for warning and red for critical presentation. Invalid values do not
raise measurement alarms; loss of CAN has its own status indication.

## User Interface

The primary DASH screen uses a dark background, large central RPM value, compact
shift-light strip, high-contrast telemetry tiles, an always-visible CAN/source
badge, and reserved alarm emphasis. It shows all values required by the
milestone at 800x480 without animation-heavy behavior.

TRACK and DIAG remain as lightweight secondary screens. SETTINGS is represented
by a separate page/module boundary so later configuration work does not require
restructuring the DASH screen. Touch provides simple page navigation. UI object
creation and update entry points remain separated so generated SquareLine files
can later replace the view layer while preserving the application-facing model.

## Display, Touch, and PSRAM

The existing `ESP32_Display_Panel` board path remains the hardware integration
layer. LVGL draw buffers prefer PSRAM and fall back to internal RAM only when a
safe allocation succeeds. Initialization verifies that the configured panel
reports 800x480. A display failure prevents UI startup and is logged. A touch
failure is logged and shown in diagnostics but does not prevent display output.

## Build and Release Artifacts

`.github/workflows/build-firmware.yml` runs on every push, pull request, and
manual dispatch. It uses stable GitHub Action major versions, installs a pinned
PlatformIO version, caches PlatformIO packages, runs native tests, builds the
Waveshare target, verifies expected outputs, and uploads one artifact named
`DIY-Dash-firmware`.

The artifact contains when generated:

- `firmware.bin`;
- `bootloader.bin`;
- `partitions.bin`;
- `boot_app0.bin`;
- `DIY-Dash-ESP32-S3-Touch-LCD-5-full.bin`;
- a flash-layout manifest or equivalent build metadata.

The merge script derives address/file pairs from PlatformIO/SCons build metadata
or the generated flash command, not from manually duplicated offsets. It invokes
the esptool package supplied by the active ESP32 platform.

## Testing Strategy

Native tests are written before new pure-logic implementation and cover:

- default/invalid `VehicleState` and atomic source switching;
- little- and big-endian signal extraction;
- signed and unsigned raw values;
- scaling and additive offset;
- short-DLC rejection and out-of-range mappings;
- per-signal and bus timeout behavior;
- CAN-to-DEMO and CAN-to-invalid transitions;
- alarm thresholds, load-gated lambda alarm, and invalid-value suppression.

The full verification sequence is:

```text
pio test -e native
pio run -e waveshare_5
repository hygiene checks
push dashboard-dev
GitHub Actions success
artifact content verification
```

Physical hardware validation remains a separate acceptance step because a
successful cross-compile cannot prove LCD timing, touch orientation, CAN bus
electrical behavior, or long-running thermal stability.

## Error Handling

- Fatal display/LVGL allocation errors stop application startup and remain
  visible in Serial logs.
- Touch initialization failure degrades gracefully.
- PSRAM absence or undersizing is explicitly reported.
- TWAI installation/start failure leaves the UI running with CAN failure status.
- Malformed or unmapped frames are counted and ignored without changing the
  current state.
- Stale signals become invalid based on their declared timeout.
- DEMO fallback never hides the underlying CAN state.

## Definition of Done

- The `dashboard-dev` branch contains no new prohibited branding and existing
  source residues are removed.
- Native tests and the target PlatformIO build pass locally.
- LCD, GT911, PSRAM, LVGL, USB Serial, and TWAI initialization are represented by
  real target code.
- Required telemetry, source state, CAN state, and alarms are rendered through
  `VehicleState`.
- The default decoder contains no assumed ECUMaster frame definitions.
- Workflow build and artifact publication complete successfully on GitHub.
- The branch is pushed and a pull request to the default branch is opened when
  repository permissions allow it.

## Next Milestone

The next milestone freezes authoritative ECUMaster EMU Black and/or EMU Classic
protocol sources, adds exact CAN IDs, offsets, byte order, types, scaling,
timeouts, validation constraints, and fixture tests, then iterates on the
motorsport UI using real vehicle data and physical-board feedback.
