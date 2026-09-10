# CAN Profiles and Shift Range Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add source-pinned CAN profile mappings and a configurable 0-10000 RPM shift-light scale with four green, four yellow, and four red segments.

**Architecture:** Static `CanProfile` tables group validated signals by complete CAN frame, and one generic decoder stages a whole frame before atomically updating normalized telemetry. The profile registry drives SETTINGS and runtime selection; the shift-light change remains an independent pure-model change. RED and GREEN verification is performed only through pushed GitHub Actions revisions.

**Tech Stack:** C++17, PlatformIO native Unity tests, Arduino/ESP32 TWAI, LVGL 8.4, GitHub Actions.

**Spec:** `docs/superpowers/specs/2026-09-08-can-profile-source-audit-and-shift-range-design.md`

## Global Constraints

- Do not run native tests or firmware builds locally; GitHub Actions is the verification authority.
- CAN reception is listen-only; add no diagnostic requests or transmitted frames.
- Preserve the numeric values of the existing twelve `ParameterId` entries.
- Profiles are compiled into firmware and selected by stable string ID.
- Unknown, malformed, out-of-range, and stale data must render invalid rather than reuse stale values.
- Link Generic Dash and PSA C2 VTS remain explicitly experimental.
- SHIFT LIGHT settings use 0-10000 RPM in 100 RPM steps and are saved on category exit.

---

### Task 1: Source ledger and RED fixtures

**Files:**
- Create: `docs/can/profile-sources.md`
- Create: `test/test_can_profile_registry/test_main.cpp`
- Create: `test/test_can_profile_fixtures/test_main.cpp`
- Modify: `test/test_shift_light_model/test_main.cpp`
- Modify: `test/test_settings_flow_model/test_main.cpp`
- Modify: `test/test_app_config/test_main.cpp`

**Interfaces:**
- Consumes: current `ParameterId`, `VehicleState`, `ShiftLightConfig`, and `EcuCanDecoder` APIs.
- Produces: executable expectations for `CanProfileRegistry::find`, atomic frame decoding, the seven profiles, fixed segment colors, and the 0-10000 range.

- [ ] **Step 1: Record the pinned source ledger**

  Add every source URL, full revision/version, profile status, frame IDs, byte order, default bitrate, and mapped parameter to `docs/can/profile-sources.md`. Mark Link and PSA experimental and explain why.

- [ ] **Step 2: Write registry and decoder fixture tests before production code**

  Define tests that request all stable IDs, assert profile metadata, decode at least one non-zero deterministic frame per mapped parameter, and assert exact normalized values. Include wrong ID, extended/standard mismatch, remote frame, wrong DLC, Link discriminator mismatch, range rejection, and a frame where one invalid signal proves that no sibling signal is updated.

- [ ] **Step 3: Write shift-light RED tests**

  Assert slider correction clamps requests to 0 and 10000, preserves ordering, and rounds to 100 RPM. Assert lit segment indices 0-3 are green, 4-7 yellow, and 8-11 red independently of configured red threshold. Assert 0 RPM can be a valid start value.

- [ ] **Step 4: Push the test-only RED commit**

  Commit only docs/tests, push `dashboard-dev`, and watch the resulting GitHub Actions run. The expected result is native compilation/test failure caused by missing registry/frame APIs and old shift limits/colors; firmware compilation may also fail because the new API is intentionally absent.

### Task 2: Atomic frame decoder and registry

**Files:**
- Create: `src/ecu/can_profile.h`
- Create: `src/ecu/can_profile_registry.h`
- Create: `src/ecu/can_profile_registry.cpp`
- Modify: `src/ecu/ecu_can_decoder.h`
- Modify: `src/ecu/ecu_can_decoder.cpp`
- Modify: `src/telemetry/telemetry_manager.h`
- Modify: `src/telemetry/telemetry_manager.cpp`
- Modify: `platformio.ini`

**Interfaces:**
- Consumes: `CanFrame`, `ParameterId`, and `VehicleState`.
- Produces: `const CanProfile* CanProfileRegistry::find(const char*)`, `profiles()`, `profileCount()`, `EcuCanDecoder::selectProfile(const CanProfile*)`, and atomic `decode`.

- [ ] **Step 1: Implement immutable profile/frame/signal types**

  Add metadata, verification status, exact DLC, optional byte discriminator, raw extraction, native bounds, timeout, and source provenance fields exactly as specified.

- [ ] **Step 2: Refactor the decoder to stage complete frames**

  Match ID/format/DLC/discriminator, bounds-check every extraction, decode endian/signed raw types, validate finite native ranges, and commit all staged values only after all checks pass.

- [ ] **Step 3: Make telemetry profile-selectable**

  Replace the constructor-only fixed decoder dependency with explicit profile selection. On selection, reset `VehicleState`, valid-frame timing, and every per-parameter timeout.

- [ ] **Step 4: Commit the decoder foundation**

  Commit source changes without claiming GREEN until GitHub Actions executes the fixtures in the later profile commit.

### Task 3: Parameter union and presentation

**Files:**
- Modify: `src/telemetry/parameter_id.h`
- Modify: `src/telemetry/parameter_registry.cpp`
- Modify: `src/telemetry/mock_telemetry.cpp`
- Modify: `src/ui/unit_presenter.h`
- Modify: `src/ui/unit_presenter.cpp`
- Modify: `test/test_parameter_registry/test_main.cpp`
- Modify: `test/test_unit_presenter/test_main.cpp`
- Modify: `test/test_mock_telemetry/test_main.cpp`

**Interfaces:**
- Consumes: appended `ParameterId` values and existing unit preferences.
- Produces: descriptors and native formatting for barometric pressure, boost target, coolant pressure, fuel temperature, ethanol, lambda 2, ignition timing, injector duty/pulse width, pedal position, MAF, EGT 1-8, and four wheel speeds.

- [ ] **Step 1: Extend tests for the exact parameter union**

  Assert every appended descriptor has a non-empty name, short name, correct native unit, sensible decimals, stable unique ID, and a safe demo value.

- [ ] **Step 2: Append stable identifiers and descriptors**

  Leave existing enum values in place, append new values before `Count`, add only required native unit categories, and extend presentation conversion without changing existing output.

- [ ] **Step 3: Extend deterministic DEMO values**

  Populate new continuous measurements with bounded deterministic values so every selectable tile is previewable in DEMO.

- [ ] **Step 4: Commit the parameter union**

  Commit source and tests together.

### Task 4: Verified ECU mapping tables

**Files:**
- Create: `src/ecu/profiles/ecumaster_emu_black.cpp`
- Create: `src/ecu/profiles/rusefi_verbose.cpp`
- Create: `src/ecu/profiles/maxxecu_default.cpp`
- Create: `src/ecu/profiles/haltech_broadcast.cpp`
- Create: `src/ecu/profiles/speeduino_haltech.cpp`
- Modify: `src/ecu/can_profile_registry.cpp`
- Modify: `test/test_can_profile_fixtures/test_main.cpp`
- Modify: `platformio.ini`

**Interfaces:**
- Consumes: `CanProfile`, `CanFrameDefinition`, `CanSignalDefinition`, and all registered parameters.
- Produces: five verified compiled profiles with source-pinned fixture coverage.

- [ ] **Step 1: Encode ECUMaster and MaxxECU tables**

  Translate only documented byte-aligned signals, convert kPa to native bar and preserve all other native units, set timeouts from documented broadcast rates, and require DLC 8.

- [ ] **Step 2: Encode rusEFI verbose tables**

  Translate the pinned DBC at base `0x200`, including its little-endian signed fields and units. Exclude DBC signals not registered for tile display.

- [ ] **Step 3: Encode Haltech document 2.0 tables**

  Translate required big-endian frames and their documented offsets/scales. Use document rates to choose freshness timeouts with sufficient scheduling margin.

- [ ] **Step 4: Encode the Speeduino compatibility subset**

  Use only bytes populated by the pinned `comms_CAN.cpp`; do not expose its explicit zero placeholders as live measurements.

- [ ] **Step 5: Complete deterministic fixtures and commit**

  Add positive and negative fixture assertions for all five profiles and commit the verified mappings.

### Task 5: Experimental Link and PSA profiles

**Files:**
- Create: `src/ecu/profiles/link_generic_dash.cpp`
- Create: `src/ecu/profiles/psa_c2_vts.cpp`
- Modify: `src/ecu/can_profile_registry.cpp`
- Modify: `test/test_can_profile_fixtures/test_main.cpp`
- Modify: `platformio.ini`

**Interfaces:**
- Consumes: frame discriminators and experimental profile metadata.
- Produces: `link_generic_dash_experimental` and `psa_c2_vts_engine_experimental`.

- [ ] **Step 1: Encode Link indexed frames**

  Use CAN ID `0x3E8`, exact DLC 8, byte-0 frame indices 0-13, byte 1 equal to zero, and only continuous channels corroborated by the official channel list. Preserve the experimental label.

- [ ] **Step 2: Encode the passive PSA subset**

  Add standard-ID DLC-8 frames `0x208` and `0x488` with RPM `0.125`, TPS `0.5`, and temperature `raw - 40` mappings. Add no VAN, transmit, diagnostics, target RPM, alerts, or non-engine-sensor frames.

- [ ] **Step 3: Add experimental negative fixtures and commit**

  Verify Link discriminator rejection, PSA wrong-DLC/extended rejection, and exact sample values; commit both profiles.

### Task 6: Runtime profile selection and SETTINGS

**Files:**
- Modify: `src/app/app.h`
- Modify: `src/app/app.cpp`
- Modify: `src/ui/ui.h`
- Modify: `src/ui/ui.cpp`
- Modify: `src/settings/app_config.cpp`
- Modify: `test/test_source_selection/test_main.cpp`
- Modify: `test/test_telemetry_manager/test_main.cpp`

**Interfaces:**
- Consumes: registry lookup/list APIs and persisted `can.profile_id`.
- Produces: populated profile dropdown, stable selection persistence, runtime reconfiguration, recommended-bitrate hint, and no-data fallback.

- [ ] **Step 1: Add selection-model tests**

  Assert verified-first ordering, experimental suffixes, exact ID persistence, invalid-ID no-data behavior, state clearing, and no automatic DEMO fallback.

- [ ] **Step 2: Populate and handle the dropdown**

  Build newline-separated LVGL options from registry metadata, select the configured ID, stage changes through the existing commit model, and display the recommended bitrate without overwriting the saved bitrate.

- [ ] **Step 3: Wire runtime reconfiguration**

  Resolve the profile before starting CAN, install it into telemetry, clear old data, and keep the UI operational if the profile is invalid or TWAI restart fails.

- [ ] **Step 4: Commit SETTINGS/runtime integration**

  Commit production changes and selection tests.

### Task 7: Shift-light 0-10000 range and fixed color quarters

**Files:**
- Modify: `src/settings/app_config.cpp`
- Modify: `src/ui/settings_flow_model.cpp`
- Modify: `src/ui/shift_light_model.cpp`
- Modify: `src/ui/ui.cpp`
- Modify: `test/test_app_config/test_main.cpp`
- Modify: `test/test_settings_flow_model/test_main.cpp`
- Modify: `test/test_shift_light_model/test_main.cpp`

**Interfaces:**
- Consumes: existing `ShiftLightConfig` fields and save-on-exit flow.
- Produces: 0-10000 slider correction/validation and fixed four-green/four-yellow/four-red positions.

- [ ] **Step 1: Implement the validated range**

  Set model validation and all LVGL sliders to 0-10000 with 100 RPM increments. Clamp loaded/candidate values while preserving `start < red < flash <= max`.

- [ ] **Step 2: Implement fixed segment colors**

  Keep progressive illumination from start to maximum, but color indices 0-3 green, 4-7 yellow, and 8-11 red. Preserve full red flashing at the existing 125 ms phase interval.

- [ ] **Step 3: Commit the shift-light change**

  Commit implementation with the RED tests from Task 1 now expected to pass.

### Task 8: GitHub GREEN verification and documentation

**Files:**
- Modify: `README.md`
- Modify: `docs/ui/dashboard-config-guide.md`
- Modify: `docs/can/profile-sources.md`

**Interfaces:**
- Consumes: final profile registry, settings UI, fixtures, and shift behavior.
- Produces: user-facing wiring/profile limitations and a traceable passing build.

- [ ] **Step 1: Push all GREEN implementation commits**

  Push `dashboard-dev`, identify the exact GitHub Actions run for the pushed SHA, and wait for native tests, Waveshare firmware build, artifact validation, and UI preview jobs.

- [ ] **Step 2: Correct failures through new RED/GREEN cycles**

  For any behavioral failure, add or retain a reproducing test in a pushed RED revision, then apply the smallest production correction and verify a later GREEN run. Do not run PlatformIO locally.

- [ ] **Step 3: Update user documentation**

  Document profile IDs/status/default bitrate, required ECU configuration, unsupported-value behavior, C2/Link experimental limitations, passive wiring warning, and the 0-10000 shift-light controls.

- [ ] **Step 4: Perform final GitHub verification**

  Confirm the final workflow SHA equals branch HEAD, download/check the generated firmware artifact metadata, inspect the generated 800x480 DASH/TRACK/SETTINGS previews, and report the workflow URL and firmware result.

- [ ] **Step 5: Commit and push documentation**

  Commit documentation, push it, and require the documentation revision's GitHub Actions run to pass before completion is claimed.
