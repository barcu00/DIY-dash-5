# Configurable Dashboard and CAN Profiles Design

## Goal

Extend the current Waveshare ESP32-S3 dashboard firmware with the approved
configurable DASH and TRACK layouts, persistent per-tile settings, configurable
full-screen warnings, a practical SETTINGS screen, verified standalone-ECU CAN
profiles, and a separate experimental engine-CAN profile for a 2005 Citroën C2
VTS.

All automated tests and firmware builds for this milestone run in GitHub
Actions. The final visual result is reviewed from the UI produced by the same
revision whose GitHub workflow passes. Physical vehicle and display validation
remain separate hardware acceptance steps.

## Scope and constraints

- Preserve the working display, touch, PSRAM, LVGL, TWAI, CI, and firmware
  artifact foundation on `dashboard-dev`.
- Reuse proven configuration and tile-model ideas from the earlier experimental
  branch selectively. Do not merge that branch wholesale.
- Keep ECU-specific frame layout and scaling out of UI code.
- The only selectable runtime data sources are `CAN` and `DEMO`. There is no
  automatic CAN-to-DEMO fallback.
- Remove the DIAG screen, menu entry, and navigation route. Minimal read-only
  CAN health information belongs in SETTINGS.
- Do not add runtime profile downloads or an arbitrary DBC importer.
- Do not introduce unrelated telemetry, networking, branding, or protocol
  changes.
- The Citroën profile is receive-only engine CAN. VAN is explicitly excluded.

## User-facing navigation

The persistent bottom navigation contains three equal-width entries:

```text
DASH | TRACK | SETTINGS
```

There is no DIAG destination. The vertical gap between the bottom-most tiles
and the navigation bar matches the compact visual gap between the shift-light
area and the tile content. Navigation must remain usable at 800x480 without
covering tiles or warning controls.

## Approved visual layouts

### DASH

```text
                    shift-light strip

  left column       center group                  right column
  4 small tiles     2 centered wide tiles         4 small tiles
                    4 small tiles in a 2 x 2 grid

                 DASH | TRACK | SETTINGS
```

- The two wide center tiles default to RPM and gear.
- A wide tile centers both its label and its value.
- RPM and gear are ordinary configurable slots: their parameter can be changed
  and the tile can be hidden.
- The four left slots, two center-wide slots, four center-small slots, and four
  right slots are four independent compacting groups. The wide group compacts
  only between its two wide positions.

The approved visual reference is
`dashboard-layout-option-a-v3.html` in the conversation visualization
workspace. Implementation should reproduce its proportions and spacing rather
than reinterpret the layout.

### TRACK

```text
                    shift-light strip

  left column       center column                 right column
  4 small tiles     4 centered wide tiles         4 small tiles

                 DASH | TRACK | SETTINGS
```

All twelve TRACK slots are configurable. The left, center, and right columns
are independent compacting groups. TRACK uses the same shift-light strip as
DASH, with identical LED count, colors, RPM progression, dimensions, spacing,
and behavior. Both screens read the same persisted start RPM, red-zone RPM, and
maximum RPM settings.

### Group compaction

Every group is bottom-anchored. When a tile is hidden, the visible tiles keep
their relative order and shift downward to occupy the lowest available slots in
that same group. A tile never moves between groups and hidden slots consume no
visible space. The behavior is deterministic for zero through all visible
tiles.

The layout engine operates on logical slots, not LVGL object order. It produces
a placement list that the view applies, which makes the packing rules testable
without display hardware.

## Tile configuration

A single tap on any visible tile opens that slot's editor. Configuration is
staged locally until the user presses Save. Cancel closes the editor without
changing runtime or persisted configuration.

Each tile editor provides:

- parameter selection;
- visible/hidden state;
- number of displayed decimal places or the supported presentation format;
- warning enabled/disabled;
- warning direction: above or below the limit;
- threshold;
- hysteresis;
- activation delay;
- Save and Cancel.

Changing the assigned parameter disables the slot's previous warning before
the new configuration is saved. This prevents a threshold intended for one
measurement from being silently applied to another.

Hidden slots remain editable through a slot list in SETTINGS for the selected
layout. The list identifies the logical group and slot, shows its assigned
parameter and visibility, and opens the same editor used by tile taps. This is
the restoration path for a hidden tile.

DASH and TRACK configurations are independent. Saving one tile writes that
slot's complete configuration and leaves every other tile unchanged. Successful
saves survive restart.

## Parameter model and presentation

The existing `VehicleState` remains the normalized boundary between decoders
and the UI. It is backed by stable parameter identifiers and descriptors:

```cpp
struct ParameterDescriptor {
    ParameterId id;
    const char* name;
    const char* short_name;
    NativeUnit native_unit;
    uint8_t default_decimals;
};

struct ParameterValue {
    float native_value;
    bool valid;
    uint32_t updated_ms;
};
```

The initial registry includes the currently supported signals: RPM, MAP,
lambda, TPS, coolant temperature, intake-air temperature, oil pressure, oil
temperature, battery voltage, vehicle speed, gear, and fuel pressure. Additional
parameters are added only when a selected CAN profile supplies a verified
mapping or when required by a supported demo fixture.

The parameter picker can show the union of registered parameters. A parameter
not supplied by the selected profile renders as `---`; the UI must never invent
or retain a stale value for it.

Unit conversion happens only in the presentation layer. Decoders and warning
evaluation use normalized native values. Supported global presentation choices
are:

- temperature: °C or °F;
- pressure: bar, kPa, or psi;
- speed: km/h or mph;
- mixture: lambda or AFR;
- configurable stoichiometric ratio for AFR conversion.

Displayed thresholds in the tile editor use the currently selected presentation
unit and are converted to native units on Save. A later unit change therefore
does not change the physical warning threshold.

## Warning behavior

Each tile owns one warning configuration:

```cpp
enum class WarningDirection : uint8_t {
    Above,
    Below,
};

struct TileWarningConfig {
    bool enabled;
    WarningDirection direction;
    float threshold_native;
    float hysteresis_native;
    uint16_t delay_ms;
};
```

Warnings are evaluated only from valid, fresh parameter values. Hiding a tile
does not disable its warning.

For an `Above` warning, activation begins when the value exceeds the threshold
for the configured delay, and the warning rearms only after the value returns
to or below `threshold - hysteresis`. For a `Below` warning, the mirrored rule
uses `threshold + hysteresis`. A value becoming invalid clears pending delay
timers and cannot trigger a warning.

When a warning activates, the dashboard shows a large red modal containing:

- `WARNING`;
- parameter name;
- current live value and unit;
- configured limit;
- an acknowledge button.

Acknowledging closes the modal, but the related tile remains red-highlighted
while the alarm condition is active. The same condition cannot open another
modal until it has returned to the safe side including hysteresis and then
crossed the limit again.

If several warnings activate, the modal shows the alarm with the greatest
normalized excursion beyond its threshold and displays the number of remaining
active alarms. Acknowledgement advances to the next unacknowledged alarm. The
selection order is deterministic when excursions are equal.

The warning modal appears above every top-level page, including SETTINGS, and
must not be obscured by navigation.

## SETTINGS

SETTINGS is a vertically scrollable top-level page with the following sections.

### Display

- software brightness from 20% through 100%;
- live preview while the control is adjusted;
- persisted value after confirmation;
- no display-off switch.

The board backlight API behaves as a switch on this hardware, so intermediate
brightness is implemented as software dimming in the LVGL presentation layer.
The persisted setting is clamped to 20-100 on load.

### Data source and CAN

- source: CAN or DEMO;
- installed CAN profile;
- bitrate: 125, 250, 500, or 1000 kbit/s;
- valid-data timeout;
- read-only CAN driver state;
- read-only accepted, ignored/malformed, and dropped frame counters when
  available.

Selecting DEMO is explicit. In CAN mode, loss of valid profile data renders
affected values as `---`; it does not switch to generated data.

Changing CAN bitrate or profile stops reception, applies the validated
configuration, resets profile-owned freshness state, and restarts the driver.
If restart fails, the UI remains operational and reports CAN unavailable.

### Shift lights

- start RPM;
- red-zone RPM;
- maximum RPM.

Save requires `start RPM < red-zone RPM <= maximum RPM`. Invalid ordering is
explained inline and is not persisted.

### Units

- temperature unit;
- pressure unit;
- speed unit;
- lambda or AFR presentation;
- stoichiometric AFR ratio.

### Layouts

- DASH slot list, including hidden slots;
- TRACK slot list, including hidden slots;
- reset DASH and TRACK layouts to approved defaults.

### System

- firmware version;
- full factory reset of application-owned settings.

Factory reset does not erase unrelated ESP32 NVS namespaces.

## Persistence

Application configuration is stored in an application-owned ESP32 NVS
namespace with an explicit schema version. Persisted state includes:

- brightness;
- data source, CAN profile, bitrate, and timeout;
- shift-light RPM values;
- unit and stoichiometric AFR choices;
- every DASH tile configuration;
- every TRACK tile configuration.

Configuration loading validates enum values, numeric ranges, string/profile
identifiers, tile counts, and shift-light ordering. Missing, corrupt, or
incompatible data falls back to safe defaults without blocking UI startup.

A tile Save is atomic from the user's perspective: validate the staged value,
write the complete updated configuration, confirm the write, then refresh the
runtime model and layout. If persistence fails, keep the previous runtime
configuration, keep the editor open, and show an error. The UI must not claim
that an unsuccessful write was saved.

## Runtime data flow

```text
CAN frame
  -> selected table-driven decoder
  -> normalized VehicleState / parameter values
  -> unit presenter
  -> DASH or TRACK tile views

normalized parameter values
  -> warning engine
  -> tile highlight and modal queue

tile tap or SETTINGS slot selection
  -> staged tile editor
  -> validation
  -> NVS save
  -> runtime configuration and group layout refresh
```

DEMO produces normalized `VehicleState` values through the same downstream
interfaces. No decoder contains UI formatting or warning policy.

## CAN profile architecture

Profiles are registered through a `CanProfileRegistry`. Each static profile
declares:

- stable profile identifier and user-facing name;
- ECU family and protocol version;
- verification status: verified or experimental;
- default bitrate;
- standard or extended frame IDs and expected DLC;
- endianness, signedness, bit/byte position, scale, offset, native unit, and
  timeout for each signal;
- source URL, source revision/version, and provenance note.

The generic decoder validates frame format, ID, DLC, extraction bounds, and
numeric conversion before updating a parameter. Malformed, short, unknown, and
out-of-range frames cannot partially update state. Profile selection clears old
values so data decoded by one profile is never presented under another.

The normal profile picker favors verified profiles and visibly marks every
experimental profile. Profiles are compiled into firmware; no network is needed
on the vehicle.

## Initial standalone ECU profiles

The milestone adds these profile entries and deterministic frame fixtures:

1. ECUMaster EMU Black standard CAN stream.
2. rusEFI CAN broadcast.
3. MaxxECU Default CAN protocol 1.3.
4. Haltech ECU Broadcast protocol 2.0.
5. Link ECU Generic Dash receive stream.
6. Speeduino configured for its documented Haltech-compatible broadcast.

Production mappings must be taken from and cross-checked against authoritative
vendor or project documentation. A profile is marked verified only when its
implemented IDs, DLC, byte order, scaling, offsets, and supported signals are
covered by fixtures derived from the pinned source version.

Gauge.S definitions may be used as an MIT-licensed supporting reference for
discovering or comparing mappings. They are translated into this project's
static profile format, cross-checked against the authoritative protocol source,
and never downloaded at runtime. A Gauge.S definition alone does not qualify a
profile or signal as verified.

Speeduino support in this milestone is explicitly the documented compatible
broadcast configuration, not an invented native stream. Signals that an
authoritative source does not define, such as an unresolved gear value in a
third-party mapping, remain unsupported and render as `---`.

### Profile sources

- ECUMaster manuals: https://www.ecumaster.com/user-manuals/
- rusEFI CAN Broadcast: https://github.com/rusefi/rusefi/wiki/CAN-Broadcast-for-Dashboards
- MaxxECU default protocol: https://www.maxxecu.com/webhelp/can-default_maxxecu_protocol.html
- Haltech ECU Broadcast: https://support.haltech.com/portal/en/kb/articles/haltech-can-ecu-broadcast-protocol
- Link CAN Gauge channels: https://kb.linkecu.com/acc-kb/latest/can-gauge-available-channels
- Speeduino CAN source documentation: https://speeduino.github.io/speeduino-doxygen/comms___c_a_n_8h_source.html
- Gauge.S reference definitions: https://github.com/handmade0octopus/gauge.s-sorek.uk/tree/master/definitions/2-Standalone

Exact document versions or Git revisions are recorded beside each implemented
profile and fixture. If the source cannot substantiate a proposed signal, that
signal is omitted rather than guessed.

## Citroën C2 VTS engine-CAN profile

Add a separately labeled profile:

```text
PSA C2 VTS CAN (experimental)
```

Target vehicle context is a 2005 Citroën C2 VTS 1.6 16V. This model year is a
transition-era architecture where the instrument-cluster area may expose VAN
for some functions even though CAN is used between powertrain controllers. The
profile therefore makes no claim that every 2005 cluster connector exposes the
required engine CAN pair.

Rules for this profile:

- receive/listen only;
- engine CAN frames only;
- no VAN decoder, VAN library, VAN wiring path, or VAN traffic;
- no diagnostic requests, frame transmission, wake-up messages, or control
  commands;
- bitrate remains user-selectable, with any default set only from corroborated
  source evidence;
- include only engine signals whose mapping is corroborated by available
  reverse-engineering sources and deterministic fixtures;
- omit uncertain fields instead of filling them from model-year assumptions;
- always display the experimental label in SETTINGS.

The primary public references are the Apache-2.0 PSA reverse-engineering
project and its archived frame documentation:

- https://github.com/prototux/PSA-RE
- https://github.com/prototux/PSA-CAN-RE-old/blob/master/FRAMES.md

`PSAVanCanBridge` may be consulted only as an architectural/vehicle-family
reference because its GPLv3 code is not to be copied into this firmware:

- https://github.com/morcibacsi/PSAVanCanBridge

This experimental profile cannot be promoted to verified until captured frames
from the target vehicle match the implemented IDs and scaling, and the displayed
values are compared with an independent trusted measurement or diagnostic
reading. Failure to find engine CAN at the chosen cluster-side connection is a
hardware/topology finding, not permission to fall back to VAN within this
milestone.

## Error and stale-data handling

- Unsupported parameters render as `---`.
- A signal exceeding its per-signal freshness timeout becomes invalid.
- Warnings ignore invalid or stale values and clear pending activation timers.
- An invalid saved profile identifier falls back to a safe no-data CAN state and
  prompts selection in SETTINGS; it does not silently select a different ECU.
- Invalid NVS configuration falls back to defaults and remains recoverable from
  SETTINGS.
- A CAN reconfiguration failure leaves display, touch, DEMO selection, and
  settings access operational.
- Brightness is clamped to 20-100%.
- Shift-light values cannot be saved unless their ordering is valid.
- Decoder errors update counters and do not mutate unrelated parameters.

## Delivery phases

### Phase A: configuration and layout model

- stable parameter registry;
- versioned application configuration and NVS store;
- independent DASH and TRACK defaults;
- bottom-anchored group compaction;
- atomic per-slot save model.

### Phase B: UI and warnings

- approved DASH and TRACK views;
- identical shift-light strip on DASH and TRACK;
- single-tap tile editor and hidden-slot restoration;
- warning state machine, modal, and tile highlighting;
- SETTINGS sections;
- software brightness, units, and shift-light configuration;
- three-entry navigation and complete DIAG removal.

### Phase C: standalone ECU profiles

- profile registry and metadata;
- six initial standalone ECU profiles;
- source-pinned deterministic fixtures;
- profile selection and CAN reconfiguration.

### Phase D: experimental C2 profile

- passive engine-CAN-only C2 profile;
- source provenance and explicit experimental UI treatment;
- fixtures only for corroborated mappings;
- documentation of the target-vehicle capture and validation procedure.

### Phase E: GitHub verification and visual acceptance

- GitHub Actions native tests;
- GitHub Actions target firmware build;
- firmware artifact validation;
- review of the final 800x480 UI generated from the passing revision;
- later physical board and vehicle acceptance.

## GitHub Actions verification

No repository tests or firmware builds for this milestone are executed in the
internal/local environment. GitHub Actions is the verification authority.

Native tests cover:

- configuration defaults, validation, schema mismatch, and serialization model;
- independent per-tile Save/Cancel behavior;
- parameter-change warning reset;
- group-local bottom compaction for DASH and TRACK, including all-hidden and
  all-visible cases;
- hidden-slot restoration model;
- unit conversions and native-threshold preservation;
- shift-light ordering validation;
- identical DASH/TRACK shift-light progression from the shared settings;
- warning direction, delay, hysteresis, acknowledgement, rearming, stale data,
  hidden tiles, multiple-alarm priority, and deterministic ties;
- every supported decoder frame fixture;
- wrong ID, wrong frame format, short DLC, extraction bounds, signedness,
  endianness, scaling, timeouts, and profile switching;
- explicit CAN/DEMO selection with no automatic fallback.

The firmware workflow also builds the Waveshare target, verifies the expected
flash artifacts, and uploads them. A passing compile does not count as physical
proof of LCD brightness quality, touch hit targets, CAN electrical connection,
or C2 frame correctness.

For visual acceptance, an 800x480 screenshot or deterministic UI preview is
produced from the passing revision and shown after CI completes. It must include
at least DASH, TRACK, SETTINGS, the tile editor, and the warning modal.

## Definition of done

- DASH and TRACK match the approved structures and compact each group downward.
- DASH and TRACK display the same shift-light strip driven by one shared
  configuration.
- Wide-tile content is centered.
- Every logical tile can change parameter, presentation, visibility, and warning
  settings through the shared editor.
- Hidden tiles can be restored and every saved tile survives restart.
- Warning modals, acknowledgement, highlighting, hysteresis, delay, priority,
  and rearming behave as specified.
- SETTINGS provides brightness, CAN/DEMO, CAN profile/configuration, shift-light
  RPM, units, layout restoration, and reset controls.
- DIAG is absent from navigation and the application.
- Six standalone ECU profiles have source-pinned fixtures; only substantiated
  mappings are exposed as verified.
- The C2 entry is passive, engine-CAN-only, contains no VAN implementation, and
  remains visibly experimental pending vehicle validation.
- GitHub Actions native tests, target build, and artifact checks pass.
- The final UI from the passing revision is presented for review.
- Physical display and target-vehicle validation steps and limitations are
  documented without being misrepresented as CI coverage.
