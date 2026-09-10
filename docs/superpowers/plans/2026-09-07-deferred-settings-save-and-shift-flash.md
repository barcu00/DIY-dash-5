# Deferred Settings Save and Shift Flash Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Remove NVS writes from LVGL callbacks, save dirty settings once on exit, add four shift-light sliders and optional full-strip red flashing, then publish verified firmware and previews as the project's first GitHub release.

**Architecture:** UI controls update the live RAM configuration and mark a small, platform-independent commit coordinator dirty. Exit paths queue the newest complete snapshot; `App::loop()` performs the repository write outside LVGL event handling and reports the result back under the display lock. Shift-light rendering receives monotonic time and derives normal or flashing segment states from one shared pure model used by DASH and TRACK.

**Tech Stack:** C++17, PlatformIO 6.1.18, Unity native tests, Arduino-ESP32/Preferences, LVGL 8.4, Python 3.12/Pillow UI previews, GitHub Actions and GitHub Releases.

**Spec:** `docs/superpowers/specs/2026-09-07-deferred-settings-save-and-shift-flash-design.md`

## Global Constraints

- Target hardware is Waveshare ESP32-S3-Touch-LCD-5 non-B at 800x480.
- Run every unit test, firmware build, preview verification, and packaging check only in GitHub Actions; do not run them in the local Codex environment.
- Keep `START RPM < RED RPM < FLASH RPM <= MAX RPM` with a 100 RPM minimum gap between strict thresholds.
- Flash the complete DASH and TRACK strip red at approximately 4 Hz using a non-blocking 125 ms half-period.
- Never write NVS from an LVGL callback.
- Preserve existing configuration through an explicit schema-v1 to schema-v2 migration.
- Keep the existing PR #1 as the review path; do not merge it unless the user separately authorizes merging.
- Publish the first release as the prerelease `v0.1.0` from the final tested `dashboard-dev` commit, with the full-board BIN, component binaries, flash layout, checksum, and UI PNGs attached.

---

## File Structure

- `src/settings/app_config.{h,cpp}` — schema-v2 shift fields, defaults, validation, and config equality.
- `src/settings/config_repository.{h,cpp}` — stored-size-aware loading and schema-v1 migration.
- `src/settings/nvs_config_backend.{h,cpp}` — expose stored blob length without reading it.
- `src/ui/settings_commit_model.{h,cpp}` — pure dirty/queued/retry state machine with no LVGL or Arduino dependency.
- `src/ui/settings_flow_model.{h,cpp}` — four-threshold correction rules.
- `src/ui/shift_light_model.{h,cpp}` — pure timed full-strip flash state.
- `src/ui/shift_light_view.{h,cpp}` — render only changed segment states.
- `src/ui/ui.{h,cpp}` — sliders, dirty staging, exit queuing, commit messages, and callback cleanup.
- `src/app/app.{h,cpp}` — execute queued repository operations outside LVGL callbacks.
- `platformio.ini` — include the new pure model in native test builds.
- `scripts/ui_preview/render_ui_preview.py` — render the new four-slider SHIFT LIGHT category.
- `scripts/tests/test_ui_preview.py` and `scripts/ui_preview/ui_contract.json` — assert the updated fixed 800x480 preview contract.
- `README.md` and `docs/ui/dashboard-config-guide.md` — current controls, save semantics, shift flash, screenshots, and flashing instructions.
- `docs/ui/screenshots/*.png` — CI-rendered final previews committed from the successful artifact.

### Task 1: Schema v2 and backward-compatible configuration loading

**Files:**
- Modify: `src/settings/app_config.h`
- Modify: `src/settings/app_config.cpp`
- Modify: `src/settings/config_repository.h`
- Modify: `src/settings/config_repository.cpp`
- Modify: `src/settings/nvs_config_backend.h`
- Modify: `src/settings/nvs_config_backend.cpp`
- Modify: `test/test_app_config/test_main.cpp`
- Modify: `test/test_config_repository/test_main.cpp`

**Interfaces:**
- Produces: `ShiftLightConfig { start_rpm, red_rpm, flash_rpm, max_rpm, flash_enabled }`.
- Produces: `ConfigBackend::storedSize() const -> std::size_t`.
- Produces: `LoadResult::Migrated` when a valid v1 blob is converted and rewritten as v2.
- Consumes: existing `ConfigRepository::saveCandidate(const AppConfig&, AppConfig&)`.

- [ ] **Step 1: Write failing schema and migration tests**

Add assertions that defaults are `5500/7000/7500/8000`, flashing is enabled, invalid four-value order is rejected, v2 round-trips, and a byte-for-byte v1 fixture preserves brightness, CAN, units, and all tile arrays while setting `flash_rpm` to the old `max_rpm` and `flash_enabled` to true. Extend `MemoryBackend` with:

```cpp
std::size_t storedSize() const override {
    return has_value ? stored_size : 0U;
}
```

- [ ] **Step 2: Push the RED tests and verify GitHub Actions fails**

Commit only the tests, push `dashboard-dev`, and poll the workflow for that commit. Expected: native compilation fails because the new fields, `storedSize()`, and `LoadResult::Migrated` do not exist.

- [ ] **Step 3: Implement schema v2, storage sizing, and migration**

Set `AppConfig::kSchemaVersion` to `2U`. Add the fields to `ShiftLightConfig`, update defaults and validation, and add an internal `LegacyAppConfigV1` in `config_repository.cpp` with the exact previous field order. Load by `backend_.storedSize()`: read v2 at `sizeof(AppConfig)`, read and validate v1 at `sizeof(LegacyAppConfigV1)`, copy every field and tile array into defaults, set the new flash fields, validate, write the v2 blob, and return `Migrated`. Unknown lengths return `DefaultsUsed`.

- [ ] **Step 4: Commit the schema implementation**

Use commit message `feat: migrate shift light settings to schema v2`.

- [ ] **Step 5: Push the GREEN implementation and verify GitHub Actions passes**

Expected: native tests, packaging tests, firmware build, preview rendering, artifact verification, and upload all succeed.

### Task 2: Deferred commit coordinator

**Files:**
- Create: `src/ui/settings_commit_model.h`
- Create: `src/ui/settings_commit_model.cpp`
- Create: `test/test_settings_commit_model/test_main.cpp`
- Modify: `platformio.ini`

**Interfaces:**
- Produces: `enum class ConfigCommitKind : uint8_t { Save, FactoryReset }`.
- Produces: `struct ConfigCommitRequest { ConfigCommitKind kind; AppConfig candidate; bool reconfigure_runtime; uint32_t revision; }`.
- Produces: `SettingsCommitModel::markDirty(bool)`, `dirty()`, `queueOnExit(const AppConfig&)`, `queueFactoryReset()`, `take(ConfigCommitRequest&)`, and `complete(uint32_t revision, bool success)`.
- Guarantees: only the newest untaken snapshot is retained; success clears dirty only for the matching revision; failure remains dirty and retryable.

- [ ] **Step 1: Write failing coordinator tests**

Cover unchanged exit, dirty exit, coalescing two edits to the latest brightness value, OR-combining runtime reconfiguration, factory reset, successful completion, failed completion, and a stale completion that must not clear a newer revision.

- [ ] **Step 2: Push the RED tests and verify GitHub Actions fails**

Expected: native compilation fails because `settings_commit_model` is absent.

- [ ] **Step 3: Implement the minimal pure state machine**

Use value copies of `AppConfig`; do not hold LVGL objects, repository pointers, callbacks, or heap allocations. Increment a monotonic `uint32_t revision_` in `markDirty()` and keep `pending_`, `in_flight_revision_`, and `dirty_` explicit.

- [ ] **Step 4: Commit the coordinator**

Use commit message `feat: add deferred settings commit coordinator`.

- [ ] **Step 5: Push the GREEN implementation and verify GitHub Actions passes**

Expected: the new native suite and all prior jobs pass.

### Task 3: Four-threshold correction and timed shift flashing

**Files:**
- Modify: `src/ui/settings_flow_model.h`
- Modify: `src/ui/settings_flow_model.cpp`
- Modify: `src/ui/shift_light_model.h`
- Modify: `src/ui/shift_light_model.cpp`
- Modify: `test/test_settings_flow_model/test_main.cpp`
- Modify: `test/test_shift_light_model/test_main.cpp`

**Interfaces:**
- Extends: `ShiftField` with `Flash`.
- Changes: `ShiftLightModel::segments(uint16_t rpm, bool rpm_valid, uint32_t now_ms, const ShiftLightConfig&) -> ShiftSegmentStates`.
- Guarantees: 125 ms red/off phases above the enabled threshold and normal progression when disabled.

- [ ] **Step 1: Replace old tests with failing four-threshold tests**

Test each slider as the edited anchor, lower/upper clamps at 1000/15000 RPM, automatic dependent moves, exact equality allowed only for `FLASH RPM == MAX RPM`, invalid RPM data, red phase at `now_ms=0`, off phase at `now_ms=125`, and red phase again at `now_ms=250`.

- [ ] **Step 2: Push the RED tests and verify GitHub Actions fails**

Expected: native compilation fails on the new enum and model signature.

- [ ] **Step 3: Implement threshold correction and flash phases**

Use 100 RPM steps and the ordered clamps. In the shift model, validate the complete invariant first. When `rpm_valid && flash_enabled && rpm >= flash_rpm`, return twelve red/lit segments for `(now_ms / 125U) % 2U == 0U` and twelve off segments otherwise; otherwise retain the current progressive calculation.

- [ ] **Step 4: Commit the behavior**

Use commit message `feat: add configurable full-strip shift flash`.

- [ ] **Step 5: Push the GREEN implementation and verify GitHub Actions passes**

Expected: all native model tests and the full workflow pass.

### Task 4: Move persistent writes out of LVGL callbacks

**Files:**
- Modify: `src/ui/ui.h`
- Modify: `src/ui/ui.cpp`
- Modify: `src/app/app.h`
- Modify: `src/app/app.cpp`
- Modify: `test/test_settings_commit_model/test_main.cpp`

**Interfaces:**
- Adds: `Ui::takeConfigCommit(ConfigCommitRequest&) -> bool`.
- Adds: `Ui::completeConfigCommit(uint32_t revision, bool success)`.
- Replaces: callback-time `persistSettings()` with RAM-only `stageSettings(AppConfig, bool)` and `queueSettingsOnExit()`.
- Consumes: `SettingsCommitModel` from Task 2.

- [ ] **Step 1: Add failing exit-path and retry state tests**

Extend the pure coordinator suite so BACK, DASH, TRACK, layout-editor close, and reset requests can be represented without calling a backend, and verify that a failed save queues again on the next exit.

- [ ] **Step 2: Push the RED tests and verify GitHub Actions fails**

Expected: at least the retry/exit contract fails against the existing coordinator behavior.

- [ ] **Step 3: Convert UI callbacks to RAM-only staging**

Every brightness, source, profile, bitrate, timeout, unit, and shift callback validates and assigns the candidate to `*config_`, performs only immediate non-persistent runtime preview work, calls `markDirty(reconfigure_runtime)`, and refreshes its own label/control. Remove direct calls to `ConfigRepository::saveCandidate()`, `resetLayout()`, and `reset()` from callbacks.

- [ ] **Step 4: Queue commits on every exit and reset path**

Call `queueSettingsOnExit()` before completing BACK/DASH/TRACK/category navigation. Convert layout reset into a staged default-array candidate and factory reset into `queueFactoryReset()`. Keep tile-editor SAVE explicit but route its commit through the same coordinator.

- [ ] **Step 5: Service persistence from `App::loop()`**

Before the next LVGL service, take one request. For `Save`, call `config_repository_.saveCandidate(request.candidate, config_)`; for `FactoryReset`, call `config_repository_.reset(config_)`. Perform these calls without `board_.lock()`. Apply runtime configuration after a successful request that requires it. Then acquire the display lock, call `completeConfigCommit()`, invalidate the active screen once, and release the lock.

- [ ] **Step 6: Commit the deferred persistence integration**

Use commit message `fix: save settings outside lvgl callbacks`.

- [ ] **Step 7: Push the GREEN implementation and verify GitHub Actions passes**

Expected: all suites pass and firmware compiles with no callback-time repository call sites in `ui.cpp`.

### Task 5: Slider-based SHIFT LIGHT screen and efficient strip view

**Files:**
- Modify: `src/ui/ui.h`
- Modify: `src/ui/ui.cpp`
- Modify: `src/ui/shift_light_view.h`
- Modify: `src/ui/shift_light_view.cpp`
- Modify: `scripts/ui_preview/render_ui_preview.py`
- Modify: `scripts/ui_preview/ui_contract.json`
- Modify: `scripts/tests/test_ui_preview.py`

**Interfaces:**
- Adds UI members for four `lv_slider` objects, four numeric labels, and one flash-enable switch.
- Changes: `ShiftLightView::update(uint16_t rpm, bool rpm_valid, uint32_t now_ms, const ShiftLightConfig&)`.
- Consumes: the timed pure model from Task 3 and `stageSettings()` from Task 4.

- [ ] **Step 1: Write failing UI contract tests**

Require the SHIFT LIGHT preview to contain `START RPM`, `RED RPM`, `FLASH RPM`, `MAX RPM`, `FLASH ENABLED`, four slider tracks, no plus/minus RPM buttons, no vertical scrolling, and exact 800x480 dimensions.

- [ ] **Step 2: Push the RED preview tests and verify GitHub Actions fails**

Expected: `Test firmware packaging` or preview contract verification fails because the fourth slider and switch are absent.

- [ ] **Step 3: Build the fixed slider screen**

Lay out four compact rows in the SETTINGS content area. Use `lv_slider_set_range(..., 1000, 15000)`, set values without animation, show `#### RPM` in right-aligned labels, and stage changes on `LV_EVENT_VALUE_CHANGED`. The switch stages `flash_enabled`. Refresh all four controls after correction so dependent threshold movement is visible immediately.

- [ ] **Step 4: Pass RPM validity and time to both strip views**

Read the RPM signal's validity from `VehicleState`, pass the same update timestamp to DASH and TRACK, cache the last `ShiftSegmentStates` in each view, and apply LVGL style changes only where `lit` or `color` differs.

- [ ] **Step 5: Update the Python preview renderer and contract**

Render four labeled sliders and the enabled switch within the existing fixed panel. Preserve all seven SETTINGS preview files and 800x480 output.

- [ ] **Step 6: Commit the screen and rendering integration**

Use commit message `feat: add shift light sliders and flash control`.

- [ ] **Step 7: Push the GREEN implementation and verify GitHub Actions passes**

Expected: native tests, firmware build, eleven previews, packaging, and artifact upload all pass.

### Task 6: Documentation and repository screenshots

**Files:**
- Modify: `README.md`
- Modify: `docs/ui/dashboard-config-guide.md`
- Create: `docs/ui/screenshots/ui-preview-dash.png`
- Create: `docs/ui/screenshots/ui-preview-track.png`
- Create: `docs/ui/screenshots/ui-preview-settings-home.png`
- Create: `docs/ui/screenshots/ui-preview-settings-display.png`
- Create: `docs/ui/screenshots/ui-preview-settings-can.png`
- Create: `docs/ui/screenshots/ui-preview-settings-shift.png`
- Create: `docs/ui/screenshots/ui-preview-settings-units.png`
- Create: `docs/ui/screenshots/ui-preview-settings-layouts.png`
- Create: `docs/ui/screenshots/ui-preview-settings-system.png`
- Create: `docs/ui/screenshots/ui-preview-tile-editor.png`
- Create: `docs/ui/screenshots/ui-preview-warning.png`

**Interfaces:**
- Consumes: CI-rendered PNGs from the successful Task 5 artifact.
- Produces: stable repository-relative image URLs usable by README and GitHub release notes.

- [ ] **Step 1: Download the successful GitHub Actions artifact without rebuilding locally**

Use the final successful workflow artifact for the exact tested commit. Verify every PNG is 800x480 and copy the generated images into `docs/ui/screenshots/` without modifying their pixels.

- [ ] **Step 2: Update README and the dashboard guide**

Document deferred save-on-exit behavior, four thresholds, the flash switch, v1-to-v2 migration, the complete artifact contents, and physical-board checks for artifact-free repeated slider movement. Embed DASH, TRACK, SETTINGS home, and SHIFT LIGHT images using repository-relative Markdown paths.

- [ ] **Step 3: Check documentation and image inventory**

Use `rg` and read-only image metadata checks locally; do not execute tests or render new previews. Confirm every Markdown image target exists and no text still claims immediate setting persistence or five preview files.

- [ ] **Step 4: Commit and push documentation**

Use commit message `docs: add dashboard previews and release guide`, then require a new successful GitHub Actions run because the release must point at the final documentation commit.

### Task 7: Final GitHub verification and first release

**Files:**
- No source changes expected.
- GitHub repository About description, PR #1 body, tag `v0.1.0`, and release metadata are updated remotely.

**Interfaces:**
- Consumes: final successful Actions run and its `DIY-Dash-firmware` artifact.
- Produces: public release `v0.1.0` and downloadable verified firmware assets.

- [ ] **Step 1: Verify the final commit exclusively in GitHub Actions**

Confirm the workflow SHA exactly matches `dashboard-dev` HEAD and every job is successful. Download its artifact, verify all expected binaries and PNGs, record the full BIN byte size, and calculate SHA-256 locally only as an artifact integrity check.

- [ ] **Step 2: Update GitHub project metadata**

Set the repository About description to `Configurable 800x480 ESP32-S3 CAN motorsport dashboard for Waveshare Touch LCD 5` and topics to `esp32-s3`, `can-bus`, `lvgl`, `motorsport`, `dashboard`, and `waveshare`. Update PR #1 with the implemented features, test workflow link, screenshots, physical-test caveat, and BIN checksum.

- [ ] **Step 3: Create release `v0.1.0` from the tested commit**

Title it `DIY Dash 5 v0.1.0 – test-board firmware`. Release notes list hardware compatibility, DASH/TRACK layouts, persistent tile editing, CAN/DEMO choice, warning modal, deferred SETTINGS saves, shift sliders/flashing, flashing address `0x0`, workflow URL, and the physical-board validation caveat. Mark it as a prerelease so it is not presented as production-ready firmware.

- [ ] **Step 4: Upload release assets**

Attach `DIY-Dash-ESP32-S3-Touch-LCD-5-full.bin`, `firmware.bin`, `bootloader.bin`, `partitions.bin`, optional `boot_app0.bin`, `flash-layout.json`, `SHA256SUMS.txt`, and all current UI preview PNGs. Update the release body to embed the attached DASH, TRACK, SETTINGS, and SHIFT LIGHT screenshots.

- [ ] **Step 5: Verify the public result**

Open the release and repository pages, confirm every asset downloads, screenshots render, the release tag resolves to the tested commit, and the About description is visible. Report the release URL, Actions URL, commit SHA, BIN size, and SHA-256 to the user.

## Plan Self-Review

- Spec coverage: deferred save, all exit paths, retry behavior, four sliders, ordering, flash timing, DASH/TRACK parity, invalid RPM, schema migration, previews, GitHub-only tests, artifact download, documentation, screenshots, and release publishing are each assigned to a task.
- Placeholder scan: no `TBD`, `TODO`, generic error-handling instruction, or undefined follow-up step remains.
- Type consistency: `ConfigCommitRequest`, `SettingsCommitModel`, the four-argument `ShiftLightModel::segments`, and the five-field `ShiftLightConfig` are defined before their consumers.
