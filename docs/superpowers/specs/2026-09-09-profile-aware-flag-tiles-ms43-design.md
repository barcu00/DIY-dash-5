# Profile-aware CAN parameters, flag tiles, and BMW MS43 Stock

Date: 2026-09-09

Status: proposed for user review

## Goal

Add a verified `BMW MS43 Stock` receive-only CAN profile, make the tile editor
offer only parameters actually supplied by the selected data source, and add
configurable boolean status tiles. Replace the tile editor overlay with a true
full-screen editor so the dashboard is not rendered behind a long parameter
list.

The explicitly rejected `BMW MS43 OLM` profile and custom `0x33C` stream are
out of scope. The implementation must not require an ECU patch.

## User-visible behavior

### Profile-aware parameter list

- `DEMO` offers every registered numeric and flag parameter.
- `CAN` offers only parameters defined by the currently selected profile.
- The list is derived from the compiled profile frames. It is not maintained as
  a second hand-written capability list.
- Changing the source or profile does not overwrite saved tile assignments.
- A tile whose saved parameter is unsupported by the active CAN profile stays
  in its original position and displays `---` with `NIEDOSTEPNY`.
- Opening such a tile still shows its saved parameter as the current selection,
  followed by the supported alternatives. The user can save a replacement or
  cancel without losing the original setting.

### Full-screen tile editor

- A two-second long press keeps opening the editor; a short tap does not.
- The editor is an independent 800x480 LVGL screen, not an object on
  `lv_layer_top()`.
- While it is open, DASH/TRACK data, shift-light, warning-modal, and background
  layout updates are paused. CAN reception and telemetry freshness continue in
  the application layer.
- `CANCEL` discards the staged changes and returns to the originating screen.
- `SAVE TILE` validates and persists the complete tile configuration, then
  returns to the originating screen.
- The parameter dropdown uses the active source/profile capability filter and
  an explicit option-index-to-`ParameterId` mapping. Enum ordinals must never be
  inferred from filtered dropdown indexes.

### Numeric tiles

Numeric tiles retain the current value, decimal, warning, and optional
temperature-bar controls. Existing saved configurations keep their behavior.

### Flag tiles

`ParameterDescriptor` identifies a flag separately from a numeric value. A flag
tile uses the approved visual variant C:

- OFF: normal dark tile with a grey `OFF` pill;
- ON: a colored left rail, subtly tinted dark background, and colored `ON` pill;
- the active color is configurable per tile: yellow, green, or red;
- visibility and parameter selection work exactly like numeric tiles;
- decimals, numeric warning threshold/hysteresis/delay, and temperature-bar
  controls are hidden because they do not apply to a boolean state;
- stale or unavailable data displays `---` and `NIEDOSTEPNY`, without an ON
  tint;
- the selected color is stored with that tile and survives restart.

The existing large red warning modal remains a numeric-threshold feature. Flag
ON styling does not open that modal, avoiding repeated full-screen interruptions
for normal states such as fuel-pump or idle activation.

## Signal model

### Stable parameter identity

Flag identifiers are appended before `ParameterId::Count`; existing numeric
values keep their current ordinals. Each new descriptor contains:

- stable ID;
- full and short labels;
- `ParameterKind::Flag`;
- `NativeUnit::None`;
- zero default decimals.

`VehicleState` continues to store floats: decoded flags are normalized to
exactly `0.0f` or `1.0f`. This keeps freshness, timeout, source switching, and
tile lookup on one established path.

### CAN extraction

`CanSignalDefinition` gains an extraction mode in addition to the existing
linear numeric transform:

- `Linear`: current raw value, scale, and bias behavior;
- `MaskedFlag`: apply a mask and shift, then evaluate a documented set of active
  raw values.

The active set is represented as a small immutable list associated with the
signal definition. This handles simple bits and Link's multi-valued feature
states without treating `armed`, `inactive`, or `disabled` as active. Decoding
is bounds-checked and transactional exactly as it is for numeric signals.

Simple one-bit flags use one active raw value (`1`). Link feature states use the
specific active enum values from the pinned decoder. No inferred or unknown bit
is exposed.

### Capabilities

A small `ParameterCapabilities` model walks every signal in a `CanProfile` and
builds a de-duplicated, registry-ordered set. It provides:

- `supports(ParameterId)` for tile rendering;
- `writeOptions(...)` plus an explicit ID array for the editor;
- `all()` for DEMO.

It allocates no heap memory and is host-testable. The UI receives the active
profile from the application instead of re-reading the settings string and
duplicating registry lookup rules.

## BMW MS43 Stock profile

Profile ID: `bmw_ms43_stock`

- Family: BMW Siemens MS43, E46 stock powertrain CAN
- Standard 11-bit identifiers
- 500 kbit/s
- Receive only
- Verification: verified against the MS4X MS43 CAN-bus documentation
- No K-line patch, OLM patch, or custom `0x33C` frame

Numeric signals:

| CAN ID | Source bytes/bits | Parameter | Conversion |
|---|---|---|---|
| `0x316` | bytes 2-3, little-endian | RPM | raw x 0.15625 |
| `0x329` | byte 1 | Coolant temperature | raw x 0.75 - 48 deg C |
| `0x329` | byte 2 | Barometric pressure | raw x 0.002 + 0.598 bar |
| `0x329` | byte 5 | Accelerator position | raw x 0.390625 % |
| `0x545` | byte 4 | Oil temperature | raw - 48 deg C |

Flag signals:

| CAN ID | Bit | Flag |
|---|---:|---|
| `0x316` byte 0 | 0 | Ignition key ON |
| `0x316` byte 0 | 1 | Crank-sensor error |
| `0x316` byte 0 | 7 | MAF error |
| `0x329` byte 3 | 0 | Clutch pressed |
| `0x329` byte 3 | 1 | Idle regulator active |
| `0x329` byte 3 | 3 | Engine running |
| `0x329` byte 6 | 0 | Brake pressed |
| `0x329` byte 6 | 1 | Brake-system fault |
| `0x329` byte 6 | 2 | Kickdown active |
| `0x545` byte 0 | 1 | Check engine / MIL |
| `0x545` byte 0 | 4 | EML |
| `0x545` byte 3 | 0 | Oil consumption warning |
| `0x545` byte 3 | 1 | Oil loss warning |
| `0x545` byte 3 | 2 | Oil sensor fault |
| `0x545` byte 3 | 3 | Coolant overheat |
| `0x545` byte 3 | 7 | Upshift request |
| `0x545` byte 7 | 7 | Low oil pressure |

Cruise-control multi-bit commands, M warm-up LED steps, and fuel-cap indication
are not exposed as simple ON/OFF engine-state tiles in this increment.

Source: [MS4X Siemens MS43 CAN Bus](https://www.ms4x.net/index.php?title=Siemens_MS43_CAN_Bus&oldid=23051).

## Flag scope for existing profiles

Only named, unambiguous states present in each pinned source are added.
Configurable generic outputs/switches and unknown/reserved bits are omitted.

### ECUMaster EMU Black

From `0x604`:

- sensor/fault flags: CLT, IAT, MAP, wideband, EGT1, EGT2, EGT-high alarm,
  knock, flex-fuel sensor, DBW, and fuel-pressure error;
- active states: gear cut, ALS, launch control, idle, traction-control
  intervention, and pit limiter.

From `0x606`: fuel pump, coolant fan, AC clutch, AC fan, nitrous, and starter
request. Table/map-set selectors, CAN switches, generic switches, parametric
outputs, and virtual outputs are intentionally omitted.

Source: [EMU Black manual](https://www.ecumaster.com/files/EMU_BLACK/EMU_BLACK_manual.pdf),
CAN Stream and bitfield footnotes, using the revision pinned in the source
ledger.

### rusEFI verbose

From base `0x200` status: rev limiter, main relay, fuel pump, check engine, O2
heater, lambda protection, fan 1, and fan 2. From base `0x20B`: brake pedal.
Launch control and anti-lag are not added because the pinned verbose DBC does
not transmit them.

Sources: pinned
[rusEFI DBC](https://github.com/rusefi/rusefi/blob/71a3239be9dbb9a06b12ba15dd8b2bdd5b8cee32/firmware/controllers/can/rusEFI_CAN_verbose.dbc)
and
[transmitter](https://github.com/rusefi/rusefi/blob/71a3239be9dbb9a06b12ba15dd8b2bdd5b8cee32/firmware/controllers/can/can_verbose.cpp).

### MaxxECU Default 1.3

From `0x526`: shift cut, rev limiter, anti-lag, launch control, traction power
limiter, throttle blip, AC/idle-up, knock, brake, clutch, speed limit, GP
limiter, user cut, ECU logging, and nitrous.

Source: [MaxxECU Default CAN output](https://www.maxxecu.com/webhelp/can-default_maxxecu_protocol.html).

### Link Generic Dash (experimental)

From frame index 12 limit flags: RPM, MAP, speed, maximum-ignition, anti-lag
ignition-cut, high-voltage, overrun, traction, low-voltage, launch-RPM,
wakeup, GP-RPM-1, closed-loop-stepper, GP-RPM-2, e-throttle, and cyclic-idle
limits/states.

From frame index 13 feature status: anti-lag active, launch-control active,
traction-control active, and cruise-control active. Each uses the documented
enum value or values, not a generic non-zero check.

Source: pinned
[LinkGenericDash decoder](https://github.com/AdaptiveEngineering/LinkGenericDash/tree/cd7426e872a40a812880e0b1c6af9c39391616c1).
The profile remains experimental because the byte layout is community-decoded.

### Haltech Broadcast 2.0, Speeduino Haltech mode, and PSA C2 VTS

No boolean flag parameters are added. Their currently pinned default-stream
sources do not provide a sufficiently clear compatible boolean state mapping:

- Haltech 2.0 includes numeric values and several enums but no verified general
  ALS/LC boolean bitfield in the implemented frame scope;
- Speeduino's pinned Haltech-compatible transmitter populates the numeric
  subset and does not substantiate additional status flags;
- PSA C2 remains limited to the reverse-engineered engine values already listed
  in the source ledger, with no VAN, body, diagnostic, or guessed bits.

These profiles still benefit from profile-aware filtering: their editor simply
shows their verified numeric parameters.

## Configuration and migration

`TileConfig` gains `FlagActiveColor`, with values `Yellow`, `Green`, and `Red`.
The stable default is yellow. `AppConfig::kSchemaVersion` increments from 3 to
4.

The repository adds an explicit schema-3 structure matching the current binary
layout. Migration copies every existing field and tile unchanged, initializes
each tile's flag color to yellow, validates the result, and writes schema 4.
Existing schema-1 and schema-2 migration paths continue to work by first
building the current configuration defaults, then applying the same new field
default. Unknown sizes or corrupt data still fall back safely to defaults.

Changing a tile from numeric to flag does not erase its dormant numeric warning
or temperature settings; those controls are hidden and ignored while the flag
is selected. Changing back restores the saved numeric settings. This avoids
surprising data loss in a staged editor.

## Rendering and update policy

Tile rendering first checks active-profile support, then freshness, then
parameter kind:

1. unsupported or stale -> unavailable presentation;
2. numeric -> existing numeric presentation and optional temperature bar;
3. flag -> OFF or ON presentation using the saved color.

A dedicated `UiActivity` state (`Dashboard`, `Track`, `Settings`, `TileEditor`)
is added to `UiUpdatePolicy`. Persistent `PageId` remains unchanged because it
addresses saved DASH/TRACK layouts. Entering `TileEditor` suppresses page,
shift-light, settings-status, and modal rendering until the editor closes.

The editor screen is created once per opening, loaded with LVGL, and deleted
after returning to the remembered origin screen. Dropdown open/close does not
rebuild the dashboard underneath it.

## Error handling

- Short DLC, unknown ID, invalid discriminator, invalid raw extraction, or
  out-of-range numeric conversion rejects the frame transactionally.
- A flag definition whose mask or active-value set is invalid is rejected by
  profile-definition tests and never reaches the runtime registry.
- Unsupported saved parameters are not replaced implicitly.
- Failed tile persistence leaves runtime configuration unchanged and keeps the
  editor open with an error message.
- An unavailable flag never renders ON.

## Test strategy

All tests and firmware builds run only in GitHub Actions, as requested. The
implementation follows red-green TDD through separate pushed commits.

The first commit adds tests expected to fail for the missing behavior:

- MS43 registry metadata and fixture decoding for every numeric and flag frame;
- masked bit and multi-value flag extraction, including malformed definitions;
- exact capability sets for every profile and all-parameter DEMO behavior;
- filtered option-index mapping and retention of an unsupported saved value;
- flag presentation OFF/ON/unavailable and all three active colors;
- flag-aware editor controls and staged save/cancel behavior;
- schema-3 to schema-4 migration preserving all old fields;
- tile-editor update suppression and return to DASH, TRACK, or SETTINGS.

After GitHub Actions demonstrates the intended failures, production code is
added in small commits until the complete native suite and firmware build are
green. The final workflow artifact must include the merged flashable `.bin` and
UI preview screenshots showing the full-screen editor plus OFF/ON flag tiles.

## Documentation deliverables

- Add BMW MS43 Stock and every implemented flag to
  `docs/can/profile-sources.md` with exact IDs, bits, revision, bitrate, and
  verification boundary.
- State explicitly that OLM, `0x33C`, VAN, and transmission/body CAN are absent.
- Update the user-facing README with profile-aware selection and flag-tile
  configuration.
- Generate updated DASH/TRACK/editor/settings screenshots through the GitHub
  workflow for later attachment to the first release.

## Acceptance criteria

- `BMW MS43 Stock` is selectable at 500 kbit/s and decodes only documented stock
  frames.
- `BMW MS43 OLM` is not present anywhere in the profile list.
- CAN tile choices exactly match the selected profile's compiled definitions;
  DEMO exposes everything.
- Unsupported saved assignments remain saved and render unavailable.
- All documented flags in the approved scope render as configurable variant-C
  tiles.
- The tile editor occupies the full display and background UI refresh is paused.
- Schema-3 user settings migrate without losing layouts, warnings, temperature
  bars, units, brightness, source, CAN settings, or shift-light settings.
- GitHub Actions is green and provides a flashable test binary plus UI previews.
