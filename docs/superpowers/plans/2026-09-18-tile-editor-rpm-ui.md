# Tile editor and RPM settings implementation plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Implement the six mockup screens approved in conversation on 2026-09-18, preserving display stability and existing configuration.

**Architecture:** Keep `Ui` as navigation/commit coordinator. Add small independently testable models for numeric entry and parameter categories, and separate LVGL editor/settings implementations from the large `ui.cpp`. Store native units in existing config; editing uses presented units with explicit conversion. Retain deferred settings commits and transactional tile drafts.

**Tech stack:** C++17, LVGL 8.4, Arduino 3.1.1-h PSRAM XIP, PlatformIO, GitHub Actions.

**Spec:** The approved six-screen mockup and preceding UI proposal in this conversation: DATA, WARNING, TEMPERATURE BAR, NUMERIC ENTRY, RPM & SHIFT LIGHT, PARAMETER PICKER.

## Global constraints

- Target Waveshare ESP32-S3-Touch-LCD-5 non-B, 800 x 480; English UI only.
- Preserve SDK XIP guards, VSYNC double buffering, 512 KiB PSRAM LVGL pool and existing six layouts.
- No local tests or compilation: run native, Python, production LVGL and ESP32-S3 tests on GitHub.
- Tile CANCEL discards all draft changes; SAVE TILE commits once and exits only after successful storage.
- Settings persist on exit, not on every slider event. Keep independent DASH/TRACK tile banks.
- Native warning threshold/hysteresis range remains 0.0 through 999.0; temperature range -999.0 through 999.0.
- RPM scale remains shared, 100 through 10000 RPM in 100-RPM increments; scale starts at zero.
- Flash remains 4 Hz; UI settings do not alter ECU rev limits.
- Existing configs remain compatible. Do not merge main or publish a release without user direction.

## Task 1: Numeric entry and unit-aware warning editing

**Files:** Create `src/ui/numeric_entry_model.h/.cpp`, `test/test_numeric_entry_model/test_main.cpp`; modify `platformio.ini`, `src/ui/tile_editor_model.h/.cpp`, `test/test_tile_editor_model/test_main.cpp`.

**Interfaces:** `NumericEntryModel::open(float value, float minimum, float maximum, uint8_t decimals)`, `append(char)`, `erase()`, `text() const`, `valid() const`, `value() const`. A decimal value is applied only when valid. Keep UI objects out of this model.

- [ ] Write RED tests for input `120.0`, one decimal separator only, empty input, bounds, erase, negative temperature, and cancellation retaining original draft.
- [ ] Add unit-aware warning tests using existing conversion APIs:

```cpp
UnitSettings units;
units.temperature = TemperatureUnit::Fahrenheit;
TEST_ASSERT_FLOAT_WITHIN(0.01f, 120.0f,
    UnitPresenter::toNative(ParameterId::OilTemperature, 248.0f, units));
```

- [ ] Push RED and confirm failures reflect missing numeric-entry behavior, not syntax errors.
- [ ] Implement bounded input storage, finite parsing, precision and range checks. Apply `RESET BELOW` as threshold minus reset value for Above; show `RESET ABOVE` and inverse calculation for Below. Convert both absolute values before deriving native hysteresis; never convert hysteresis with an absolute Fahrenheit offset.
- [ ] Verify GitHub GREEN including PSI/kPa/AFR and reset-side validation; commit.

## Task 2: Full-screen editor tabs and numeric keypad

**Files:** Create `src/ui/ui_tile_editor.cpp`, `src/ui/ui_numeric_entry.cpp`; modify `src/ui/ui.h`, `src/ui/ui.cpp`, `design/production-preview/transitions.cpp`, `.github/workflows/production-view-previews.yml` and existing Python editor-contract tests.

**Interfaces:** Editor navigation remains `Ui::openEditor(TileAddress)`, `closeEditor()` and `saveEditor()`. Add private `showEditorTab(uint8_t)` and `openNumericEntry(lv_obj_t* target)`; keypad APPLY updates a draft control, not NVS. Retain old controls only where needed for existing production test interaction; update tests to exercise actual new navigation.

- [ ] Add real LVGL RED scenarios asserting tab selection, absence of temperature controls for non-temperature parameters, keypad cancel/apply, editor cancel, successful save and failed-save retry.
- [ ] Push and inspect expected RED on GitHub.
- [ ] Build header, fixed footer and DATA/WARNING/TEMPERATURE BAR panels. Render only active panel. Show ACTIVE COLOR for flags, hide numeric options for flags. Preview the draft tile with cached telemetry, without updating dashboard behind editor.
- [ ] Implement numeric-entry screen with 3x4 keypad, decimal key, backspace, APPLY/CANCEL, unit and range feedback. Disable APPLY for invalid input; restore originating tab on close.
- [ ] Show specific invalid-field messages, units and a reset summary. Disable irrelevant settings when their feature is off. Reset decimals to descriptor defaults when parameter changes.
- [ ] Run GitHub native/contracts/real-LVGL tests and inspect 800x480 screenshots for clipping and centered data; commit.

## Task 3: Categorized parameter selection

**Files:** Modify `src/ui/parameter_options.h/.cpp`, `test/test_parameter_options/test_main.cpp`, `src/ui/ui_tile_editor.cpp`, `src/ui/ui.h`, `design/production-preview/transitions.cpp`.

**Interfaces:** Add `enum class ParameterCategory { Engine, Temperature, Pressure, Flags }` and `ParameterOptions::category(ParameterId)`. Build each picker category by filtering existing capability-aware options; never enable an unsupported parameter merely because it belongs to a category.

- [ ] Write RED tests mapping Celsius to Temperature, Bar to Pressure, flags to Flags and other numeric units to Engine; assert CAN capability filtering survives category filtering.
- [ ] Push and inspect RED.
- [ ] Implement full-screen category tabs and paginated touch rows showing full name/unit. Retain existing unavailable selection visibly but prevent newly selecting unsupported data. SELECT applies to the editor draft; BACK leaves it unchanged.
- [ ] Verify selection, returning to prior editor tab, category with no options and numeric/flag transitions in real LVGL on GitHub; commit.

## Task 4: Unified RPM and shift-light settings

**Files:** Create `src/ui/ui_rpm_settings.cpp`; modify `src/ui/ui.h`, `src/ui/ui.cpp`, `src/ui/settings_flow_model.h/.cpp`, `test/test_settings_flow_model/test_main.cpp`, `design/production-preview/transitions.cpp`.

**Interfaces:** Reuse `SettingsCategory::ShiftLight` to avoid enum/storage migration. Relocate `rpm_scale_slider_` and its value from LAYOUTS into RPM & SHIFT LIGHT. Keep existing `ShiftField` and stored `shift.max_rpm`; expose its historical independent value only under an explicitly labeled advanced setting, rather than silently overwriting it.

- [ ] Add RED tests for shared scale bounds/steps, edited RPM via keypad, inconsistent thresholds and no unintended configuration changes on cancellation.
- [ ] Push and inspect RED.
- [ ] Render RPM preview, editable slider rows RPM SCALE MAX/YELLOW FROM/RED FROM/FLASH FROM, enabled switch and 4 Hz label. Use numeric keypad for exact values. Keep LAYOUTS focused on layout/tile assignment.
- [ ] Show conflicts if a threshold is beyond the visible scale; do not silently rewrite saved thresholds. Preserve current 12-LED 4/4/4 behavior for compatibility and explain its independent fill maximum in advanced settings. Changing LED color semantics is a separate approval, not hidden in a UI refactor.
- [ ] Verify saving on exit, cancellation of numeric entry, retained settings after repository reload and first correct destination frame on GitHub; commit.

## Task 5: Warning test and complete workflow verification

**Files:** Modify `src/ui/ui_tile_editor.cpp`, `src/ui/ui.h`, `src/app/app.cpp`, `src/alarms/buzzer_model.h/.cpp`, `test/test_buzzer/test_main.cpp`, `design/production-preview/transitions.cpp`, host board stub only as needed.

**Interfaces:** Add a bounded user-requested test event consumed by App; do not inject fake telemetry or alter warning-engine acknowledgement/state. TEST WARNING displays a clearly labeled preview and requests a short buzzer pulse when warning sound is enabled; startup test remains unchanged.

- [ ] Write RED tests for a single bounded pulse, disabled warning sound, repeated press and precedence of actual warnings.
- [ ] Push and inspect RED.
- [ ] Implement test modal with current draft value/unit and a close action; preserve actual warnings. Never mark a real warning acknowledged when closing a test.
- [ ] Run all GitHub tests and compile target. Capture six real production screens in partial/direct modes, compare results and inspect actual screenshots. Check no dashboard rendering resumes behind editor/keypad/picker.
- [ ] Verify full-image packaging and SHA-256; download candidate BIN and request read-only review. Do not claim physical performance verified by host tests.
- [ ] Update `docs/ui/dashboard-config-guide.md`, README screenshots and candidate validation notes. Hand off full BIN with offset `0x0` and NVS reset warning; ask hardware test of editor, repeated saves, RPM changes and shift-light flashing.

## Plan self-review

All six accepted screens map to Tasks 2–4; unit editing and reset semantics to Task 1; test-warning interaction to Task 5. Existing storage fields suffice. SDK/display changes, CAN profile expansion, SD, new alarm bounds and new LED color algorithms are deliberately excluded. Completion requires GitHub evidence and visual inspection, followed by separately reported user hardware verification.
