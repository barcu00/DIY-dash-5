# CAN Profile Source Audit and Shift Range Design

## Goal

Implement the approved compiled-in CAN profiles without guessing frame layouts,
and adjust the shared DASH/TRACK shift-light configuration to a 0-10000 RPM
range with twelve segments arranged as four green, four yellow, and four red.

## Source policy

- Vendor documentation is authoritative for ECUMaster, MaxxECU, and Haltech.
- Project-owned DBC/source is authoritative for rusEFI and Speeduino.
- Link's official channel list confirms supported channel names, while the
  pinned community decoder supplies the byte layout. The Link profile is
  therefore visibly experimental until the byte layout is cross-checked
  against an official Link stream definition or hardware capture.
- PSA-RE is reverse-engineered rather than manufacturer documentation. The C2
  profile is always experimental and passive.
- Gauge.S definitions are comparison material only. They are not copied and
  cannot independently promote a mapping to verified.
- Every repository source is pinned to a full commit SHA. Vendor PDFs/pages are
  pinned by document/protocol version and publication date where available.

## Pinned sources

| Profile | Authoritative source | Pinned revision |
|---|---|---|
| ECUMaster EMU Black | EMU Black User Manual, CAN Stream | document 1.4, firmware 2.169+, 2026-07-09 |
| rusEFI verbose | `rusEFI_CAN_verbose.dbc` and `can_verbose.cpp` | `71a3239be9dbb9a06b12ba15dd8b2bdd5b8cee32` |
| MaxxECU | Default CAN output protocol | version 1.3, 2020-09-29 |
| Haltech | ECU Broadcast CAN Protocol | document version 2.0 |
| Link Generic Dash | official available-channel list plus LinkGenericDash decoder | `cd7426e872a40a812880e0b1c6af9c39391616c1` |
| Speeduino Haltech-compatible | `comms_CAN.h/.cpp` and `speeduino.ini` | `5275fbaf82e57b364d9e0d0b4cbadbc33f95c668` |
| PSA C2 VTS experimental | PSA-RE AEE2004 HS.IS YAML | `74294e99bd8f4decbfcdceabb11c7413dd977f4d` |
| PSA supporting archive | PSA-CAN-RE-old DBC/docs | `d09c912bf6c53b9f24a6963358a75b447f40abc5` |
| Gauge.S comparison | standalone definitions | `594208bf648dfd115fbfd639e5b7718451dffad5` |

## Profile status and initial mappings

### Verified vendor/project profiles

The following profiles are marked verified because their byte layout is
defined by the pinned producer documentation:

- `ecumaster_emu_black`: default base `0x600`, standard 11-bit frames,
  little-endian, default 1 Mbit/s. It supplies RPM, TPS, IAT, MAP, vehicle
  speed, oil temperature, oil pressure, fuel pressure, coolant temperature,
  lambda, gear, and battery voltage.
- `rusefi_verbose`: default base `0x200`, standard 11-bit frames, little-endian,
  user-configurable ECU base address but fixed dashboard profile base. It
  supplies RPM, gear, speed, TPS, MAP, coolant and intake temperature, oil
  pressure and temperature, battery voltage, lambda, and fuel pressure.
- `maxxecu_default_1_3`: IDs `0x520-0x537`, standard 11-bit frames,
  little-endian, 500 kbit/s. It supplies every current core dashboard
  parameter, including gear, oil pressure/temperature, and fuel pressure.
- `haltech_broadcast_2_0`: standard 11-bit big-endian broadcast frames,
  1 Mbit/s default. It supplies the core channels present in document 2.0.
- `speeduino_haltech`: IDs and byte layout emitted by Speeduino's selected
  Haltech broadcast mode. Only fields actually populated by the pinned source
  are exposed; source-coded zero placeholders remain unsupported.

### Experimental profiles

- `link_generic_dash_experimental`: one configurable base CAN ID carrying
  fourteen indexed eight-byte frames. The frame index is byte 0, byte 1 must
  be zero, and payload values start at byte 2. The default compiled profile
  listens at decimal ID 1000 (`0x3E8`) and documents that the ECU stream ID must
  match. The profile is experimental because Link's public page confirms
  channels but not the pinned byte packing used by the decoder.
- `psa_c2_vts_engine_experimental`: 500 kbit/s 11-bit AEE2004 HS.IS receive-only
  profile. It accepts only `0x208` and `0x488`: RPM and throttle from `0x208`;
  coolant temperature, oil temperature, and intake-air temperature from
  `0x488`. It omits VAN, diagnostics, transmission, ABS/wheel-speed frames,
  unknown fields, alerts, and the uncertain target/idle RPM field.

The C2 bitrate and applicability to a specific 2005 vehicle must be confirmed
on hardware. No transmission is ever initiated by this firmware.

## Decoder architecture

Replace the flat signal-only table with two nested immutable tables:

```cpp
struct CanSignalDefinition {
    ParameterId parameter;
    uint8_t byte_offset;
    RawType raw_type;
    ByteOrder byte_order;
    float scale;
    float bias;
    float minimum_native;
    float maximum_native;
    uint32_t timeout_ms;
};

struct CanFrameDefinition {
    uint32_t can_id;
    bool extended;
    uint8_t expected_dlc;
    uint8_t discriminator_offset;
    uint8_t discriminator_mask;
    uint8_t discriminator_value;
    const CanSignalDefinition* signals;
    size_t signal_count;
};
```

A discriminator mask of zero disables discriminator matching. The decoder
first validates frame format, exact DLC, discriminator bounds, and every signal
extraction. It then computes all values into a small stack staging array,
validates finite values and native ranges, and only then updates
`VehicleState`. A rejected frame cannot partially mutate state.

`CanProfileRegistry` owns static profile metadata and frame tables. Lookup is by
stable string ID. Selecting a profile resets all signal validity and rebuilds
per-parameter timeouts. Invalid IDs resolve to the existing no-data profile;
they never silently select another ECU.

## Parameter registry expansion

Keep the existing twelve numeric IDs unchanged and append dashboard-useful
numeric parameters verified by at least one implemented source: barometric
pressure, boost target, coolant pressure, fuel temperature, ethanol content,
lambda 2, ignition timing, injector duty, injector pulse width, accelerator
position, mass air flow, EGT 1-8, and individual wheel speeds.

Raw error codes, protocol counters, status bitfields, limiter flags, and unknown
channels are not selectable tiles in this milestone. They are not continuous
measurements and need a separate status/event UI design. An unsupported
parameter remains invalid and renders `---`.

## SETTINGS integration

The CAN profile dropdown lists verified profiles first, followed by names with
an explicit `(experimental)` suffix. A profile selection is staged in the
existing settings model and persisted on leaving the category. Applying it
stops TWAI, resolves the profile, clears old state, installs its decoder and
timeouts, then restarts TWAI using the selected bitrate.

The profile's documented default bitrate is metadata, not an automatic
override of a user's saved bitrate. A short hint shows the recommended bitrate.

## Shift-light behavior

- All four sliders have a supported range of 0-10000 RPM in 100 RPM steps.
- Ordering remains `start < red < flash <= maximum`, with a 100 RPM minimum
  gap where strict ordering is required.
- The strip remains progressive between configurable start and maximum RPM.
- Segment color is fixed by physical position: indices 0-3 green, 4-7 yellow,
  and 8-11 red. Thresholds control illumination and flashing, not segment color.
- The optional full-strip red flash at/above `flash_rpm` remains shared by DASH
  and TRACK and keeps the existing fast 125 ms phase interval.
- Existing valid saved values above 10000 are clamped deterministically while
  preserving ordering on load; no unrelated setting is reset.

## GitHub-only verification

No native test or firmware build is executed locally. TDD uses separate pushed
commits: a test-only RED commit must fail in GitHub Actions for the intended
missing behavior, followed by implementation commits that make the same tests
and the target firmware build pass. Test fixtures cover positive mappings,
wrong IDs/formats, exact DLC, discriminators, signedness, byte order, range
rejection, atomicity, profile switching, stale values, dropdown status, and the
0-10000 RPM/four-per-color shift-light rules.

## Hardware acceptance limits

GitHub fixtures prove deterministic decoding, not electrical connectivity or
vehicle correctness. Final C2 acceptance requires passive capture on the target
car and comparison of RPM/temperatures/TPS with a trusted diagnostic reading.
The Link profile also remains experimental until compared with an ECU capture
or an official byte-layout file.
