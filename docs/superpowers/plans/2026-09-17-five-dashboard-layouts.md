# Five Dashboard Layouts Implementation Plan

> **For agentic workers:** Use superpowers:executing-plans inline, as requested by the user. Track steps with checkboxes; do not delegate.

**Goal:** Add three approved presets and independently assign any of five layouts to DASH and TRACK without losing saved settings.

**Architecture:** Retain default legacy banks, add four alternate banks per page, and resolve active tiles through one accessor. A shared layout model determines geometry and visible slot counts. A dedicated custom-drawn RPM view avoids allocating tick/segment objects.

**Tech Stack:** C++17, Arduino ESP32-S3, LVGL 8.4, PlatformIO, Unity, Pillow, GitHub Actions.

**Spec:** `docs/superpowers/specs/2026-09-17-five-dashboard-layouts.md`

## Global Constraints

- 800×480, English UI, no DIAG page or new Bluetooth functionality.
- Shared RPM scale from zero to a positive configurable maximum up to 10,000, 100-RPM steps.
- Preserve old layouts, tile editor, warnings, temperature bars, flag colors, units, and NVS schemas 1–5.
- Settings persist on exit; no flash writes during slider gestures.
- All test execution and firmware compilation happen on GitHub Actions.
- Work in the existing development checkout; no main merge or release.

### Task 1: Persistence and preset model

Files: `src/settings/app_config.{h,cpp}`, `src/settings/config_repository.cpp`, `src/ui/dashboard_layout.{h,cpp}`, `src/ui/tile_engine.cpp`, native tests, `platformio.ini`.

- [x] Add a v5 persisted fixture test before implementation. Its migration must return `Migrated`, preserve the hidden Speed tile and Oil T warning, and write current schema.
```cpp
TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(LoadResult::Migrated),
                       static_cast<uint8_t>(repository.load(loaded)));
TEST_ASSERT_FALSE(loaded.dash_tiles[0].visible);
TEST_ASSERT_FLOAT_WITHIN(0.001f, 110.5f,
                        loaded.track_tiles[10].warning.threshold_native);
```
- [x] Push tests and observe the assertion failure on GitHub Actions against the old implementation.
- [x] Implement enum `DashboardLayout`, `selectedLayout`, `activeTiles`, `layoutTiles`, `dashboardLayoutName`, and `dashboardLayoutSlotCount`.
- [x] Add schema 6 banks and shared `rpm_scale_max`; validate enum bounds and clamp/snap scale without modifying shift thresholds.
- [x] Freeze legacy bank sizes at 14/12 and implement migration 5 → 6 without resetting warnings/bars/flags.
- [x] Exercise page independence, tile-bank retention, layouts fitting 800×430, hidden-column packing, and old schemas in Unity tests. Re-run on GitHub.

### Task 2: Views and controls

Files: `src/ui/rpm_scale_view.{h,cpp}`, `src/ui/tile_view.{h,cpp}`, `src/ui/ui.{h,cpp}`, `src/ui/settings_flow_model.{h,cpp}`, `src/alarms/tile_warning_engine.{h,cpp}`, `src/ui/tile_editor_model.cpp`, `src/lv_conf.h`.

- [x] Add RPM presentation model tests: at 3,000 on a 6,000 scale fill is 500/1000; above maximum clamp to 1000; invalid data uses zero fill.
```cpp
TEST_ASSERT_EQUAL_UINT16(500U, rpmScaleFill(3000.0f, true, 6000U));
TEST_ASSERT_EQUAL_UINT16(1000U, rpmScaleFill(11000.0f, true, 10000U));
TEST_ASSERT_EQUAL_UINT16(0U, rpmScaleFill(3000.0f, false, 6000U));
```
- [x] Build custom-drawn analog gauge and horizontal/curved bars with static scales and small dynamic needle/fill layers. Reuse views, do not recreate screen objects on data updates.
- [x] Add compact row, hero numeric, and gear tile geometries with centered white values and adaptive fonts. Preserve temperature/flag rendering and long press.
- [x] Add separate preset selectors in LAYOUTS and shared RPM SCALE MAX slider. Keep paginated tile controls on the same fixed-size screen.
- [x] Route editor and warnings through active banks. Prevent dormant presets and out-of-layout slots from raising warnings. Reset restores all banks only for the requested page.
- [x] Draw navigation icons with LVGL primitives so icon glyph availability does not affect output.

### Task 3: GitHub verification and handoff

Files: `scripts/ui_preview/render_ui_preview.py`, `scripts/ui_preview/ui_contract.json`, `scripts/tests/test_ui_preview.py`, `README.md`, `docs/ui/dashboard-config-guide.md`.

- [x] Extend deterministic previews for the three new layouts, updated navigation, and preset settings. Assert all previews are 800×480 and generated.
- [x] Push final implementation to development branch and wait for native tests, Python tests, firmware build, and merged full BIN verification on GitHub.
- [x] Inspect actual CI logs and resulting previews. Review persistence sizes, out-of-range accesses, warning bank changes, and repaint costs before handoff.
- [x] Download CI full BIN and preview artifacts. Clearly distinguish deterministic visual previews from hardware screenshots.
- [x] Report tested commit, GitHub run, and full BIN path. Document that hardware smoothness/touch validation still requires the physical board.

## Verification record

- RED: run 35204403090 failed the new schema-5 migration assertion before implementation.
- GREEN: commit `f3a94f8`, GitHub Actions run [35206114189](https://github.com/barcu00/DIY-dash-5/actions/runs/35206114189).
- Native suite: 184 cases passed. Python packaging/preview suite: 24 tests passed.
- ESP32-S3 build, 16 deterministic previews, and merged 16-MB full image succeeded.
- Full image SHA-256: `DA8A3D7C981D2058B5D39993ECC0597962C32DFE40964DCCCF25A9810F704239`.
- Artifact downloaded under workspace `artifacts/layouts-f3a94f8/` and previews inspected visually.
- Independent read-only review found one compact flag-row overlap; fixed in tested commit `f3a94f8`.
- No main merge or public release. Physical board smoothness, clipping, touch, and vehicle CAN testing remain required.
- Implementation ran inline. A bounded independent reviewer was used only as mandated by requesting-code-review.
