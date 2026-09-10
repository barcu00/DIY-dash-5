# Profile-aware flag tiles and BMW MS43 Stock implementation plan

> Execute this plan in the current task. All test and firmware commands run in
> GitHub Actions only; local commands are limited to source inspection, editing,
> Git checks, and artifact inspection.

**Goal:** Add a verified stock BMW MS43 profile, profile-filtered tile choices,
configurable boolean flag tiles, and a smooth full-screen English-only tile
editor while preserving existing saved settings.

**Architecture:** Keep one stable parameter registry and one `VehicleState` for
numeric and boolean data. Extend compiled CAN signal definitions with a bounded
masked-flag transform, derive profile capabilities directly from those signal
definitions, and map filtered dropdown indexes explicitly back to stable
`ParameterId` values. Add pure host-testable models for flag presentation and
editor/update behavior; LVGL only renders those decisions.

**Toolchain:** C++17, PlatformIO native Unity tests, Arduino ESP32-S3 firmware,
LVGL 8.4, Python/Pillow UI previews, GitHub Actions.

**Approved design:**
`docs/superpowers/specs/2026-09-09-profile-aware-flag-tiles-ms43-design.md`

## Invariants

- Do not add `BMW MS43 OLM`, custom `0x33C`, K-line patch dependencies, VAN,
  body CAN, or transmission CAN.
- Do not guess undocumented bits.
- Do not renumber any existing `ParameterId`.
- Do not replace unsupported saved tile assignments automatically.
- Do not place Polish text in any user-visible UI string.
- Do not run tests, builds, firmware packaging, or preview rendering locally.
- Each red commit must be observed failing in GitHub Actions for the expected
  missing behavior before its production implementation is pushed.
- Each green phase must finish with both push and pull-request workflow runs
  passing before moving to the next red phase.

## Phase 1 — CAN flag primitive, registry metadata, and BMW MS43

### Task 1: Add failing tests for flag descriptors and masked extraction

**Modify:**

- `test/test_parameter_registry/test_main.cpp`
- `test/test_ecu_can_decoder/test_main.cpp`
- `test/test_can_profile_registry/test_main.cpp`
- `test/test_can_profile_fixtures/test_main.cpp`

**Test first:**

- Existing 35 numeric `ParameterId` ordinals remain unchanged.
- Newly appended status IDs report `ParameterKind::Flag`, `NativeUnit::None`,
  and zero decimals.
- `MaskedFlag` extracts a single bit to exactly `0.0f`/`1.0f`.
- `MaskedFlag` supports an active-value bitset for multi-valued states.
- A flag definition with zero mask, invalid shift, extracted value above 63, or
  empty active-value set rejects the whole frame without partially changing
  `VehicleState`.
- Linear decoding remains byte-for-byte compatible.
- Registry contains `bmw_ms43_stock`, verified, standard 11-bit, 500000 bit/s,
  with the pinned MS4X source and no OLM profile.
- MS43 fixtures cover `0x316`, `0x329`, and `0x545`, including simultaneous
  numeric and bit signals, ON and OFF transitions, range checks, short DLC, and
  unrelated IDs.

Use the exact documented fixture arithmetic:

- RPM raw little-endian x 0.15625;
- coolant raw x 0.75 - 48 deg C;
- barometric raw x 0.002 + 0.598 bar;
- accelerator raw x 0.390625 percent;
- oil temperature raw - 48 deg C.

Commit the tests as `test: define CAN flag and MS43 behavior`, push, and watch
the GitHub workflow fail because the new API/profile is not implemented.

### Task 2: Implement the CAN flag primitive and BMW profile

**Modify:**

- `src/telemetry/parameter_id.h`
- `src/telemetry/parameter_registry.h`
- `src/telemetry/parameter_registry.cpp`
- `src/ecu/can_profile.h`
- `src/ecu/ecu_can_decoder.h`
- `src/ecu/ecu_can_decoder.cpp`
- `src/ecu/profiles/profile_declarations.h`
- `src/ecu/can_profile_registry.cpp`
- `platformio.ini`

**Create:**

- `src/ecu/profiles/bmw_ms43_stock.cpp`

Implementation details:

- Append all approved flag IDs before `Count` and introduce
  `ParameterKind { Numeric, Flag }` in each descriptor.
- Append defaulted fields to `CanSignalDefinition` so every existing aggregate
  initializer continues to compile:
  `CanSignalKind kind`, `uint32_t raw_mask`, `uint8_t raw_shift`, and
  `uint64_t active_values_mask`.
- For `MaskedFlag`, read the same bounded raw width, apply mask/shift, require an
  extracted value in 0..63, and use its bit in `active_values_mask` to produce
  exact 0/1.
- Validate definition invariants before staging any value.
- Raise the fixed staged-signal capacity from 8 to at least 24 because one CAN
  frame can expose many independent bits. Keep storage on the stack and reject
  definitions above the bound.
- Implement only the approved MS43 numeric and flag table from the design.
- Add the profile after the existing verified profiles and before the
  experimental profiles, preserving the registry's verified-first contract.
  Persisted selection remains authoritative by profile ID, not index.

Commit as `feat: add masked CAN flags and BMW MS43 stock`, push, and require both
GitHub workflow runs to pass.

## Phase 2 — Verified flag definitions for existing profiles

### Task 3: Add failing fixtures for every approved profile flag

**Modify:**

- `test/test_can_profile_fixtures/test_main.cpp`
- `test/test_can_profile_registry/test_main.cpp`

Add source-derived fixtures and exact capability expectations for:

- ECUMaster `0x604`: 11 named error bits and six active-state bits;
- ECUMaster `0x606`: fuel pump, coolant fan, AC clutch, AC fan, nitrous, starter;
- rusEFI `0x200`: rev limiter, main relay, fuel pump, CEL, O2 heater, lambda
  protection, fan 1, fan 2;
- rusEFI `0x20B`: brake pedal;
- MaxxECU `0x526`: all 15 documented non-reserved states;
- Link frame index 12: all 16 named limit flags;
- Link frame index 13: exact active values for ALS, launch, traction, and cruise.

Also assert that Haltech, Speeduino, and PSA expose no flag descriptors in their
compiled frames, and that reserved/generic/uncertain bits never appear.

Commit as `test: define verified profile flag mappings`, push, and confirm the
expected failing GitHub result.

### Task 4: Add the approved profile flag definitions

**Modify:**

- `src/ecu/profiles/ecumaster_emu_black.cpp`
- `src/ecu/profiles/rusefi_verbose.cpp`
- `src/ecu/profiles/maxxecu_default.cpp`
- `src/ecu/profiles/link_generic_dash.cpp`

Use only the pinned source bits documented in the design. For Link feature
statuses, encode exact active enum values in `active_values_mask`; do not use a
generic non-zero predicate. Leave Haltech, Speeduino, and PSA unchanged.

Keep every frame definition below the decoder's fixed signal capacity and use
the existing per-signal timeout conventions.

Commit as `feat: decode verified ECU status flags`, push, and require both
GitHub runs to pass.

## Phase 3 — Profile-derived capabilities and safe filtered options

### Task 5: Add failing capability and option-mapping tests

**Create:**

- `test/test_parameter_capabilities/test_main.cpp`

**Modify:**

- `test/test_parameter_options/test_main.cpp`
- `platformio.ini`

Tests must prove:

- DEMO capability contains every `ParameterId` exactly once in registry order.
- Each CAN capability is the de-duplicated union of its compiled frame signals.
- Haltech/Speeduino/PSA remain numeric-only.
- MS43 offers exactly its approved numeric and flag signals.
- `supports(id)` handles null profile and invalid IDs safely.
- A filtered option list writes English short names and carries a parallel ID
  array; selection maps through that array, never an enum cast.
- If a tile's saved ID is unsupported, it is inserted once as the current first
  option and labeled with ` (UNAVAILABLE)`.
- Truncation returns failure without unterminated text or out-of-range mapping.

Commit as `test: define profile-aware parameter choices`, push, and observe the
expected GitHub failure.

### Task 6: Implement profile capabilities and mapped options

**Create:**

- `src/ecu/parameter_capabilities.h`
- `src/ecu/parameter_capabilities.cpp`

**Modify:**

- `src/ui/parameter_options.h`
- `src/ui/parameter_options.cpp`
- `platformio.ini`

Use fixed `std::array<ParameterId, parameterCount()>` storage and a count; no
heap allocation. Build the CAN set by walking `profile.frames[].signals[]`, then
emit in global registry order for stable UI. Provide the explicit dropdown
index mapping and optional unavailable-current entry.

Commit as `feat: derive tile choices from CAN profiles`, push, and require both
GitHub runs to pass.

## Phase 4 — Configuration migration and flag presentation

### Task 7: Add failing schema and flag-presentation tests

**Create:**

- `test/test_flag_tile_model/test_main.cpp`

**Modify:**

- `test/test_app_config/test_main.cpp`
- `test/test_config_repository/test_main.cpp`
- `test/test_tile_editor_model/test_main.cpp`
- `test/test_tile_view_policy/test_main.cpp`
- `platformio.ini`

Tests must prove:

- `TileConfig` persists `FlagActiveColor::{Yellow,Green,Red}`.
- Defaults and invalid enum normalization choose yellow.
- An exact schema-3 blob migrates to schema 4 while preserving source,
  brightness, CAN, shift light, units, all 26 tiles, warnings, and temperature
  bars.
- Existing schema-1 and schema-2 paths still migrate to schema 4.
- Flag OFF uses dark base plus grey OFF pill.
- Flag ON uses the selected rail/pill color and the corresponding subtle tint.
- Unsupported or stale flags render unavailable and never retain ON styling.
- Flag editor changes are staged, saved, canceled, and validated.
- Switching parameter kind hides/ignores inapplicable controls without deleting
  dormant numeric settings.

Commit as `test: define flag tile configuration and presentation`, push, and
observe the expected GitHub failure.

### Task 8: Implement schema 4 and the pure flag tile model

**Create:**

- `src/ui/flag_tile_model.h`
- `src/ui/flag_tile_model.cpp`

**Modify:**

- `src/settings/app_config.h`
- `src/settings/app_config.cpp`
- `src/settings/config_repository.cpp`
- `src/ui/tile_editor_model.h`
- `src/ui/tile_editor_model.cpp`
- `src/ui/tile_view_policy.h`
- `src/ui/tile_view_policy.cpp`
- `platformio.ini`

Add `FlagActiveColor` to `TileConfig`, bump schema to 4, and define an exact
legacy schema-3 tile/app structure. Centralize migration copying so old fields
cannot be dropped. Normalize invalid color values.

The pure flag model returns semantic presentation colors/states only; it does
not depend on LVGL. Preserve dormant numeric settings across kind switches and
disable their runtime effect whenever the active descriptor is a flag.

Commit as `feat: persist and present configurable flag tiles`, push, and require
both GitHub runs to pass.

## Phase 5 — Full-screen editor and rendering integration

### Task 9: Add failing update-policy and source integration tests

**Modify:**

- `test/test_ui_update_policy/test_main.cpp`
- `test/test_tile_engine/test_main.cpp`
- `test/test_tile_view_policy/test_main.cpp`
- `scripts/tests/test_deferred_settings_save.py`

**Create:**

- `scripts/tests/test_ui_english_only.py`

Tests must prove:

- `UiActivity::TileEditor` blocks DASH/TRACK data, shift-light, settings-status,
  layout, and warning-modal drawing.
- Telemetry data itself remains untouched while rendering is paused.
- Closing restores the remembered DASH, TRACK, or SETTINGS activity.
- Unsupported profile assignments produce English `UNAVAILABLE` presentation.
- The editor uses a real LVGL screen and no longer creates its root on
  `lv_layer_top()`.
- Dropdown selection is resolved through the option ID mapping.
- The editor uses the long-press event only.
- User-visible strings added or changed by this feature contain no Polish text
  or Polish diacritics; explicitly ban Polish variants of unavailable, save,
  cancel, warning, settings, and parameter.

Commit as `test: define full-screen English tile editor`, push, and observe the
expected GitHub failure.

### Task 10: Implement the full-screen English editor and live tiles

**Modify:**

- `src/app/app.h`
- `src/app/app.cpp`
- `src/ui/ui.h`
- `src/ui/ui.cpp`
- `src/ui/ui_update_policy.h`
- `src/ui/ui_update_policy.cpp`
- `src/ui/tile_engine.h`
- `src/ui/tile_engine.cpp`
- `src/ui/tile_view.h`
- `src/ui/tile_view.cpp`

Implementation details:

- Pass the active `DataSource` and `CanProfile*` into the UI capability context.
- Replace `editor_overlay_` with an independent `editor_screen_` created from
  `lv_obj_create(nullptr)` and loaded through LVGL.
- Remember the origin activity and screen; return deterministically after save
  or cancel, and destroy only the editor screen.
- Set `UiActivity::TileEditor` before loading the screen, clear pending modal
  visuals, and suppress background rendering until return.
- Populate the dropdown from the filtered `ParameterOptions` mapping and map
  changes back through its ID array.
- Reconfigure the editor sections when the selected parameter kind changes:
  numeric controls for numeric values; active-color buttons for flags.
- Render variant C through the pure flag presentation result: 4-6 px left rail,
  subtle tint, right-aligned ON/OFF pill, centered content, and no stale color.
- Keep every added visible string in English.

Commit as `feat: add full-screen profile-aware tile editor`, push, and require
both GitHub runs to pass.

## Phase 6 — Sources, previews, and final GitHub artifact

### Task 11: Update source ledger and user documentation

**Modify:**

- `docs/can/profile-sources.md`
- `README.md`

Document:

- exact BMW MS43 stock IDs, fields, 500 kbit/s, MS4X oldid/revision, and
  receive-only behavior;
- every approved flag by profile and source;
- explicit no-flag conclusions for Haltech, Speeduino, and PSA;
- `DEMO = all`, `CAN = active-profile only`;
- preservation and `UNAVAILABLE` behavior;
- variant-C flag color configuration;
- no OLM, `0x33C`, VAN, body, or transmission scope;
- full-screen editor and English-only UI.

Check all links and pinned revisions by inspection. Commit as
`docs: document profile-aware CAN flag tiles` and push.

### Task 12: Extend UI previews and verify the final artifact on GitHub

**Modify:**

- `scripts/ui_preview/render_ui_preview.py`
- `scripts/tests/test_ui_preview.py`
- `.github/workflows/build-firmware.yml` only if new preview filenames are not
  already collected by the existing wildcard.

Add deterministic 800x480 previews for:

- DASH with mixed numeric, OFF, and ON flag tiles;
- TRACK with the same approved layout rules;
- full-screen numeric tile editor;
- full-screen flag tile editor with yellow/green/red selector;
- unsupported saved parameter showing `UNAVAILABLE`.

Add preview assertions first in the same commit if they fail against the old
renderer; observe the expected GitHub failure, then update the renderer and
push the green implementation.

For final verification:

1. Confirm push and PR workflow runs are green.
2. Download the `DIY-Dash-firmware` artifact from the final commit.
3. Verify artifact contents without executing firmware: `firmware.bin`,
   `bootloader.bin`, `partitions.bin`, full merged `.bin`,
   `flash-layout.json`, and all preview PNGs.
4. Inspect every preview visually for English-only text, clipping, centering,
   correct variant-C colors, and no dashboard visible behind the editor.
5. Record the final commit SHA, GitHub run IDs, artifact name, merged binary
   filename, and memory usage reported by the firmware build.
6. Leave the worktree clean.

## Completion report

Report only evidence from the final GitHub runs:

- implemented profile/flag scope and explicit exclusions;
- schema migration and preservation behavior;
- native-test and firmware-build status with links/run IDs;
- flash/RAM usage from PlatformIO output;
- downloadable artifact and exact test-board `.bin`;
- embedded or linked preview screenshots;
- remaining hardware-validation boundary for MS43, Link, and PSA.
