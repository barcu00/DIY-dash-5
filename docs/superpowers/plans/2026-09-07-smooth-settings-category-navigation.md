# Smooth Settings Category Navigation Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Replace the long scrolling SETTINGS page with six fixed category screens that autosave changes and remain responsive on the 800x480 Waveshare display.

**Architecture:** Add a small platform-independent settings-flow model for category state, layout pagination, commit timing, and Shift Light correction. Keep LVGL object ownership in `Ui`, but create only the current SETTINGS screen and destroy it on category navigation; reuse the existing tile editor and repository. Extend the Python preview contract so GitHub Actions verifies the new landing, category, layout, and confirmation states before building and packaging firmware.

**Tech Stack:** C++17, Arduino/ESP32-S3, LVGL 8.4, PlatformIO Unity tests, Python 3.12/Pillow preview tests, GitHub Actions.

**Spec:** `docs/superpowers/specs/2026-09-07-settings-category-navigation-design.md`

## Global Constraints

- Target board: Waveshare ESP32-S3-Touch-LCD-5 non-B, 800x480.
- SETTINGS contains no vertically scrolling mega-page.
- Only the active SETTINGS category owns LVGL controls and periodic visual updates.
- Discrete controls persist on valid value change; sliders persist on release.
- CAN processing and alarm evaluation continue while SETTINGS is open.
- Existing DASH/TRACK layout, warning modal, tile editor behavior, and 80-line double buffers remain unchanged.
- Source choices remain CAN and DEMO only.
- All tests and firmware builds run in GitHub Actions, never in the local Codex environment.
- Every RED or GREEN verification step pushes the commit and inspects the workflow for that exact SHA.

**Execution amendment:** The planned source-text contract was replaced during
plan review with behavior checks against the rendered 800x480 SETTINGS outputs,
the platform-independent flow model, and the compiled Waveshare firmware. This
avoids a brittle test that merely searches implementation text.

---

## File Structure

- Create `src/ui/settings_flow_model.h`: category, input-event, reset-target, Shift-field enums and the platform-independent navigation/pagination API.
- Create `src/ui/settings_flow_model.cpp`: deterministic model implementation and Shift Light correction.
- Create `test/test_settings_flow_model/test_main.cpp`: native unit coverage for model behavior.
- Modify `platformio.ini`: include `settings_flow_model.cpp` in the native test build.
- Modify `src/settings/app_config.cpp`: require strict `start < red < max` ordering to match the approved design.
- Modify `test/test_app_config/test_main.cpp`: update validation expectations for strict Shift Light ordering.
- Modify `src/ui/ui.h`: replace long-page SETTINGS members with category-screen lifecycle and confirmation state.
- Modify `src/ui/ui.cpp`: implement the six screens, autosave events, pagination, reset confirmation, and active-screen-only updates.
- Modify `src/ui/ui_update_policy.h` and `src/ui/ui_update_policy.cpp`: represent active control interaction rather than scroll interaction.
- Modify `test/test_ui_update_policy/test_main.cpp`: verify status/modal update suspension during control interaction.
- Modify `scripts/ui_preview/render_ui_preview.py`: render the approved SETTINGS states.
- Modify `scripts/tests/test_ui_preview.py`: assert the complete preview set and 800x480 dimensions.
- Modify `scripts/ui_preview/ui_contract.json`: add category grid, content bounds, and layout page constants.
- Modify `docs/ui/dashboard-config-guide.md`: document category navigation and autosave timing.

---

### Task 1: Settings Flow Model and Strict Shift Ordering

**Files:**
- Create: `src/ui/settings_flow_model.h`
- Create: `src/ui/settings_flow_model.cpp`
- Create: `test/test_settings_flow_model/test_main.cpp`
- Modify: `src/settings/app_config.cpp`
- Modify: `test/test_app_config/test_main.cpp`
- Modify: `platformio.ini`

**Interfaces:**
- Consumes: `AppConfig`, `PageId`, and `ShiftLightConfig` from `settings/app_config.h`.
- Produces: `SettingsCategory`, `SettingsInputKind`, `SettingsInputEvent`, `SettingsResetTarget`, `ShiftField`, and `SettingsFlowModel`.
- Produces methods: `open(SettingsCategory)`, `backToHome()`, `category() const`, `selectLayout(PageId)`, `layout() const`, `pageIndex() const`, `pageCount() const`, `firstSlot() const`, `nextPage()`, `previousPage()`, `shouldPersist(SettingsInputKind, SettingsInputEvent)`, and `correctedShift(ShiftLightConfig, ShiftField, uint16_t)`.

- [ ] **Step 1: Write the failing model and validation tests**

Create tests that include these cases:

```cpp
void test_navigation_starts_home_and_returns_home() {
    SettingsFlowModel model;
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(SettingsCategory::Home),
                            static_cast<uint8_t>(model.category()));
    model.open(SettingsCategory::DataCan);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(SettingsCategory::DataCan),
                            static_cast<uint8_t>(model.category()));
    model.backToHome();
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(SettingsCategory::Home),
                            static_cast<uint8_t>(model.category()));
}

void test_layout_pages_are_six_slots_and_page_is_clamped() {
    SettingsFlowModel model;
    model.selectLayout(PageId::Dash);
    TEST_ASSERT_EQUAL_UINT32(3U, model.pageCount());
    TEST_ASSERT_EQUAL_UINT32(0U, model.firstSlot());
    TEST_ASSERT_TRUE(model.nextPage());
    TEST_ASSERT_EQUAL_UINT32(6U, model.firstSlot());
    TEST_ASSERT_TRUE(model.nextPage());
    TEST_ASSERT_EQUAL_UINT32(12U, model.firstSlot());
    TEST_ASSERT_FALSE(model.nextPage());
    model.selectLayout(PageId::Track);
    TEST_ASSERT_EQUAL_UINT32(2U, model.pageCount());
    TEST_ASSERT_EQUAL_UINT32(0U, model.pageIndex());
}

void test_discrete_changes_commit_immediately_and_sliders_on_release() {
    TEST_ASSERT_TRUE(SettingsFlowModel::shouldPersist(
        SettingsInputKind::Discrete, SettingsInputEvent::ValueChanged));
    TEST_ASSERT_FALSE(SettingsFlowModel::shouldPersist(
        SettingsInputKind::Slider, SettingsInputEvent::ValueChanged));
    TEST_ASSERT_TRUE(SettingsFlowModel::shouldPersist(
        SettingsInputKind::Slider, SettingsInputEvent::Released));
}

void test_shift_change_keeps_strict_order() {
    ShiftLightConfig shift{5500U, 7000U, 8000U};
    shift = SettingsFlowModel::correctedShift(shift, ShiftField::Start, 7900U);
    TEST_ASSERT_EQUAL_UINT16(7900U, shift.start_rpm);
    TEST_ASSERT_EQUAL_UINT16(8000U, shift.red_rpm);
    TEST_ASSERT_EQUAL_UINT16(8100U, shift.max_rpm);
    shift = SettingsFlowModel::correctedShift(shift, ShiftField::Maximum, 6000U);
    TEST_ASSERT_TRUE(shift.start_rpm < shift.red_rpm);
    TEST_ASSERT_TRUE(shift.red_rpm < shift.max_rpm);
}
```

Change the existing equal-red/max validation test to expect invalid:

```cpp
config.shift = ShiftLightConfig{5500U, 7000U, 7000U};
const ValidationResult result = config.validate();
TEST_ASSERT_FALSE(result.valid);
TEST_ASSERT_FALSE(result.shift_order_valid);
```

- [ ] **Step 2: Commit and verify RED on GitHub Actions**

```powershell
git add platformio.ini src/settings/app_config.cpp test/test_app_config/test_main.cpp test/test_settings_flow_model/test_main.cpp
git commit -m "test: define settings category flow"
git push origin dashboard-dev
$sha = git rev-parse HEAD
gh run list --workflow build-firmware.yml --commit $sha --limit 1
gh run watch (gh run list --workflow build-firmware.yml --commit $sha --limit 1 --json databaseId --jq '.[0].databaseId') --exit-status
```

Expected: GitHub Actions fails in `Run native unit tests` because `settings_flow_model.h` and its implementation do not exist yet, or because strict red/max validation is not implemented.

- [ ] **Step 3: Add the model interface**

Create `src/ui/settings_flow_model.h` with this public contract:

```cpp
#pragma once
#include <cstddef>
#include <cstdint>
#include "settings/app_config.h"

enum class SettingsCategory : uint8_t { Home, Display, DataCan, ShiftLight, Units, Layouts, System };
enum class SettingsInputKind : uint8_t { Discrete, Slider };
enum class SettingsInputEvent : uint8_t { ValueChanged, Released };
enum class SettingsResetTarget : uint8_t { DashLayout, TrackLayout, Factory };
enum class ShiftField : uint8_t { Start, Red, Maximum };

class SettingsFlowModel {
public:
    static constexpr std::size_t kSlotsPerPage = 6U;
    void open(SettingsCategory category);
    void backToHome();
    SettingsCategory category() const;
    void selectLayout(PageId page);
    PageId layout() const;
    std::size_t pageIndex() const;
    std::size_t pageCount() const;
    std::size_t firstSlot() const;
    bool nextPage();
    bool previousPage();
    static bool shouldPersist(SettingsInputKind kind, SettingsInputEvent event);
    static ShiftLightConfig correctedShift(ShiftLightConfig current,
                                           ShiftField field,
                                           uint16_t requested_rpm);
private:
    SettingsCategory category_ = SettingsCategory::Home;
    PageId layout_ = PageId::Dash;
    std::size_t page_index_ = 0U;
};
```

- [ ] **Step 4: Implement deterministic pagination and correction**

In `settings_flow_model.cpp`, use 100 RPM separation and the supported 1000-15000 RPM range. Preserve the edited field whenever possible: editing Start pushes Red and Maximum upward, editing Red moves Start down and Maximum up, and editing Maximum pulls Red and Start downward. Clamp the edited field to `1000..14800`, `1100..14900`, or `1200..15000` respectively so all three fields remain valid.

Change `AppConfig::validate()` to:

```cpp
result.shift_order_valid =
    shift.start_rpm < shift.red_rpm && shift.red_rpm < shift.max_rpm;
```

Add `+<ui/settings_flow_model.cpp>` to the native `build_src_filter`.

- [ ] **Step 5: Commit and verify GREEN on GitHub Actions**

```powershell
git add platformio.ini src/ui/settings_flow_model.* src/settings/app_config.cpp test/test_app_config/test_main.cpp test/test_settings_flow_model/test_main.cpp
git commit -m "feat: add settings flow model"
git push origin dashboard-dev
$sha = git rev-parse HEAD
gh run watch (gh run list --workflow build-firmware.yml --commit $sha --limit 1 --json databaseId --jq '.[0].databaseId') --exit-status
```

Expected: native tests, Python tests, Waveshare firmware build, preview rendering, and artifact collection all pass.

---

### Task 2: Active SETTINGS Screen Lifecycle and Autosave

**Files:**
- Modify: `src/ui/ui.h`
- Modify: `src/ui/ui.cpp`
- Modify: `src/ui/ui_update_policy.h`
- Modify: `src/ui/ui_update_policy.cpp`
- Modify: `test/test_ui_update_policy/test_main.cpp`
- Create: `scripts/tests/test_settings_source_contract.py`

**Interfaces:**
- Consumes: `SettingsFlowModel` from Task 1 and existing `ConfigRepository::saveCandidate`.
- Produces private `Ui` methods: `showSettings(SettingsCategory)`, `destroySettingsScreen()`, `createSettingsHome()`, `createDisplaySettings()`, `createDataCanSettings()`, `createShiftSettings()`, `createUnitSettings()`, `createLayoutSettings()`, `createSystemSettings()`, `persistSettings(AppConfig, bool)`, and `showSettingsMessage(const char*)`.

- [ ] **Step 1: Write failing policy and source-contract tests**

Extend `test/test_ui_update_policy/test_main.cpp`:

```cpp
void test_control_interaction_suspends_nonessential_visual_updates() {
    UiUpdatePolicy policy;
    policy.activate(PageId::Settings);
    policy.setInteractionActive(true);
    TEST_ASSERT_FALSE(policy.shouldUpdateSettingsStatus(1000U));
    TEST_ASSERT_FALSE(policy.allowModalUpdates());
    policy.setInteractionActive(false);
    TEST_ASSERT_TRUE(policy.shouldUpdateSettingsStatus(1001U));
}
```

Create `scripts/tests/test_settings_source_contract.py` to assert that the former long scroll implementation is absent and all category factories exist:

```python
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]

class SettingsSourceContractTest(unittest.TestCase):
    def test_settings_uses_fixed_category_screens(self):
        source = (ROOT / "src/ui/ui.cpp").read_text(encoding="utf-8")
        self.assertNotIn("LV_DIR_VER", source)
        self.assertNotIn("SAVE SETTINGS", source)
        for symbol in (
            "createSettingsHome", "createDisplaySettings", "createDataCanSettings",
            "createShiftSettings", "createUnitSettings", "createLayoutSettings",
            "createSystemSettings",
        ):
            self.assertIn(symbol, source)
```

- [ ] **Step 2: Commit and verify RED on GitHub Actions**

```powershell
git add test/test_ui_update_policy/test_main.cpp scripts/tests/test_settings_source_contract.py
git commit -m "test: require fixed settings category screens"
git push origin dashboard-dev
$sha = git rev-parse HEAD
gh run watch (gh run list --workflow build-firmware.yml --commit $sha --limit 1 --json databaseId --jq '.[0].databaseId') --exit-status
```

Expected: `Test firmware packaging` fails because `ui.cpp` still contains `LV_DIR_VER`, `SAVE SETTINGS`, and none of the category factory names.

- [ ] **Step 3: Replace persistent long-page state with active-screen state**

In `ui.h`, remove `settings_container_`, `settingsScrollEvent`, the all-category widget set, and the full 26-entry `layout_labels_` array. Add:

```cpp
SettingsFlowModel settings_flow_{};
lv_obj_t* settings_ = nullptr;
lv_obj_t* settings_message_ = nullptr;
lv_obj_t* settings_status_ = nullptr;
std::array<lv_obj_t*, SettingsFlowModel::kSlotsPerPage> layout_labels_{};
std::array<std::size_t, SettingsFlowModel::kSlotsPerPage> layout_slots_{};
```

Make `load(Page::Settings)` call `showSettings(SettingsCategory::Home)`. `showSettings` creates a new LVGL root, builds only the selected category, attaches shared navigation, loads the new root with no animation, and deletes the previous SETTINGS root after the screen switch. Clear all category-specific widget pointers when destroying a screen.

- [ ] **Step 4: Build home, DISPLAY, DATA & CAN, SHIFT LIGHT, and UNITS screens**

Use header Y `20`, content bounds `x=16, y=66, width=768, height=348`, and navigation Y `430`. Clear `LV_OBJ_FLAG_SCROLLABLE` on every screen and content panel.

Bind dropdowns/spinbox step buttons to `LV_EVENT_VALUE_CHANGED` or `LV_EVENT_CLICKED` and call `persistSettings` immediately. Bind the brightness slider to two events:

```cpp
lv_obj_add_event_cb(brightness_slider_, settingsEvent,
                    LV_EVENT_VALUE_CHANGED,
                    reinterpret_cast<void*>(BrightnessPreview));
lv_obj_add_event_cb(brightness_slider_, settingsEvent,
                    LV_EVENT_RELEASED,
                    reinterpret_cast<void*>(BrightnessCommit));
```

`BrightnessPreview` changes only the software brightness. `BrightnessCommit` copies the displayed value into a candidate and calls `persistSettings` once.

For Shift Light buttons, call `SettingsFlowModel::correctedShift`, update all three controls, then persist the corrected candidate. For source, bitrate, timeout, or CAN profile changes, set `runtime_reconfigure_requested_ = true` only after successful persistence.

- [ ] **Step 5: Implement persistence feedback and safe failed writes**

`persistSettings` uses the repository as the single commit point:

```cpp
bool Ui::persistSettings(AppConfig candidate, bool reconfigure_runtime) {
    if (!repository_ || !config_ ||
        !repository_->saveCandidate(candidate, *config_)) {
        showSettingsMessage("SAVE ERROR");
        return false;
    }
    runtime_reconfigure_requested_ |= reconfigure_runtime;
    board_->setSoftwareBrightness(config_->brightness_percent);
    showSettingsMessage("SAVED");
    return true;
}
```

Do not rebuild the active screen inside this function. The event handler updates only controls affected by normalization. Message replacement updates one existing label and must not open a modal.

- [ ] **Step 6: Commit and verify GREEN on GitHub Actions**

```powershell
git add src/ui/ui.h src/ui/ui.cpp src/ui/ui_update_policy.* test/test_ui_update_policy/test_main.cpp scripts/tests/test_settings_source_contract.py
git commit -m "feat: add autosaving settings category screens"
git push origin dashboard-dev
$sha = git rev-parse HEAD
gh run watch (gh run list --workflow build-firmware.yml --commit $sha --limit 1 --json databaseId --jq '.[0].databaseId') --exit-status
```

Expected: all native/Python tests and the Waveshare firmware build pass. The exact SHA run must show a successful firmware artifact upload.

---

### Task 3: Paginated Layouts and Confirmed System Resets

**Files:**
- Modify: `src/ui/ui.h`
- Modify: `src/ui/ui.cpp`
- Modify: `src/settings/config_repository.h`
- Modify: `src/settings/config_repository.cpp`
- Modify: `test/test_config_repository/test_main.cpp`
- Modify: `scripts/tests/test_settings_source_contract.py`

**Interfaces:**
- Consumes: `SettingsFlowModel::kSlotsPerPage`, selected `PageId`, page index, and `SettingsResetTarget`.
- Produces repository methods `resetLayout(PageId page, AppConfig& runtime_config)` and existing `reset(AppConfig&)` remains the factory reset.
- Produces private `Ui` methods `refreshLayoutPage()`, `openResetConfirmation(SettingsResetTarget)`, `closeResetConfirmation()`, and `confirmReset()`.

- [ ] **Step 1: Write failing repository and UI contract tests**

Add repository tests using the existing fake backend:

```cpp
void test_reset_dash_layout_preserves_track_and_other_settings() {
    AppConfig runtime = AppConfig::defaults();
    runtime.brightness_percent = 40U;
    runtime.dash_tiles[0].visible = false;
    runtime.track_tiles[0].visible = false;
    TEST_ASSERT_TRUE(repository.resetLayout(PageId::Dash, runtime));
    TEST_ASSERT_TRUE(runtime.dash_tiles[0].visible);
    TEST_ASSERT_FALSE(runtime.track_tiles[0].visible);
    TEST_ASSERT_EQUAL_UINT8(40U, runtime.brightness_percent);
}
```

Add the symmetric TRACK test and failure test proving runtime configuration is unchanged when backend save fails. Extend the Python source contract to require `PREVIOUS`, `NEXT`, `RESET DASH`, `RESET TRACK`, and a confirmation factory.

- [ ] **Step 2: Commit and verify RED on GitHub Actions**

```powershell
git add test/test_config_repository/test_main.cpp scripts/tests/test_settings_source_contract.py
git commit -m "test: define paged layouts and scoped resets"
git push origin dashboard-dev
$sha = git rev-parse HEAD
gh run watch (gh run list --workflow build-firmware.yml --commit $sha --limit 1 --json databaseId --jq '.[0].databaseId') --exit-status
```

Expected: native compilation fails because `ConfigRepository::resetLayout` is absent, and/or the Python contract fails because the new controls do not exist.

- [ ] **Step 3: Implement scoped repository reset**

Implement `resetLayout` by copying only the selected default array into a candidate, then calling `saveCandidate`:

```cpp
bool ConfigRepository::resetLayout(PageId page, AppConfig& runtime_config) {
    AppConfig candidate = runtime_config;
    const AppConfig defaults = AppConfig::defaults();
    if (page == PageId::Dash) candidate.dash_tiles = defaults.dash_tiles;
    else if (page == PageId::Track) candidate.track_tiles = defaults.track_tiles;
    else return false;
    return saveCandidate(candidate, runtime_config);
}
```

- [ ] **Step 4: Implement the six-slot layout page**

Create exactly `min(6, remaining slots)` slot buttons. Store each absolute slot number in `layout_slots_`; do not derive the selected address from the button's current child index. PREVIOUS/NEXT call the model, then rebuild only the LAYOUTS screen. Disable PREVIOUS on page zero and NEXT on the last page. Opening a slot calls the existing `openEditor({selected_page, slot})`. After `saveEditor`, rebuild the current LAYOUTS page so its parameter and visibility text reflect the committed configuration.

- [ ] **Step 5: Implement reset confirmation**

SYSTEM exposes separate `RESET DASH`, `RESET TRACK`, and `FACTORY RESET` buttons. Each calls `openResetConfirmation(target)` and creates one top-layer panel with target-specific text plus CANCEL/CONFIRM. CONFIRM calls `resetLayout` for a layout or `reset` for factory, marks layout dirty after success, shows `SAVED` or `SAVE ERROR`, and closes the confirmation panel. Factory reset applies brightness/runtime configuration and returns to the SETTINGS home rather than unexpectedly loading DASH.

- [ ] **Step 6: Commit and verify GREEN on GitHub Actions**

```powershell
git add src/ui/ui.h src/ui/ui.cpp src/settings/config_repository.* test/test_config_repository/test_main.cpp scripts/tests/test_settings_source_contract.py
git commit -m "feat: page layout settings and confirm resets"
git push origin dashboard-dev
$sha = git rev-parse HEAD
gh run watch (gh run list --workflow build-firmware.yml --commit $sha --limit 1 --json databaseId --jq '.[0].databaseId') --exit-status
```

Expected: all test stages, firmware build, and artifact upload pass for the exact commit SHA.

---

### Task 4: Build-Matched SETTINGS Previews and User Documentation

**Files:**
- Modify: `scripts/ui_preview/ui_contract.json`
- Modify: `scripts/ui_preview/render_ui_preview.py`
- Modify: `scripts/tests/test_ui_preview.py`
- Modify: `docs/ui/dashboard-config-guide.md`

**Interfaces:**
- Consumes: approved category names and 800x480 bounds.
- Produces preview artifacts `ui-preview-settings-home.png`, `ui-preview-settings-display.png`, `ui-preview-settings-can.png`, `ui-preview-settings-shift.png`, `ui-preview-settings-units.png`, `ui-preview-settings-layouts.png`, and `ui-preview-settings-system.png`.

- [ ] **Step 1: Write failing preview contract tests**

Replace the single SETTINGS preview expectation with:

```python
SETTINGS_PREVIEWS = (
    "ui-preview-settings-home.png",
    "ui-preview-settings-display.png",
    "ui-preview-settings-can.png",
    "ui-preview-settings-shift.png",
    "ui-preview-settings-units.png",
    "ui-preview-settings-layouts.png",
    "ui-preview-settings-system.png",
)

def test_settings_renderer_creates_every_fixed_screen(self):
    with tempfile.TemporaryDirectory() as output:
        render_all(Path(output))
        for name in SETTINGS_PREVIEWS:
            image = Image.open(Path(output) / name)
            self.assertEqual((800, 480), image.size)
```

Also assert from `ui_contract.json` that `settings.content_height == 348`, `settings.scrollable is false`, `settings.category_count == 6`, and `settings.layout_slots_per_page == 6`.

- [ ] **Step 2: Commit and verify RED on GitHub Actions**

```powershell
git add scripts/tests/test_ui_preview.py
git commit -m "test: require fixed settings previews"
git push origin dashboard-dev
$sha = git rev-parse HEAD
gh run watch (gh run list --workflow build-firmware.yml --commit $sha --limit 1 --json databaseId --jq '.[0].databaseId') --exit-status
```

Expected: `Test firmware packaging` fails because the seven new SETTINGS images and settings contract fields are absent.

- [ ] **Step 3: Implement previews and documentation**

Add this contract object:

```json
"settings": {
  "content_x": 16,
  "content_y": 66,
  "content_width": 768,
  "content_height": 348,
  "scrollable": false,
  "category_count": 6,
  "layout_slots_per_page": 6
}
```

Render each screen using the firmware's labels, spacing, colors, selected states, and bottom navigation. The LAYOUTS preview shows DASH page 1/3 with six real default slot parameters. Update the user guide to state that list/dropdown changes save immediately, brightness saves on release, tile changes save through the tile editor, and resets require confirmation.

- [ ] **Step 4: Commit and verify final GREEN build**

```powershell
git add scripts/ui_preview/ui_contract.json scripts/ui_preview/render_ui_preview.py scripts/tests/test_ui_preview.py docs/ui/dashboard-config-guide.md
git commit -m "docs: add smooth settings previews and guide"
git push origin dashboard-dev
$sha = git rev-parse HEAD
$run = gh run list --workflow build-firmware.yml --commit $sha --limit 1 --json databaseId --jq '.[0].databaseId'
gh run watch $run --exit-status
gh run view $run --json url,conclusion,headSha
```

Expected: conclusion `success`, `headSha` equals `$sha`, and the artifact contains the firmware BIN files plus all UI previews.

- [ ] **Step 5: Download and verify the GitHub artifact**

```powershell
$artifact = "artifacts-from-github/$sha"
New-Item -ItemType Directory -Force -Path $artifact | Out-Null
gh run download $run --name DIY-Dash-firmware --dir $artifact
Get-FileHash "$artifact/DIY-Dash-ESP32-S3-Touch-LCD-5-full.bin" -Algorithm SHA256
Get-ChildItem $artifact -Filter 'ui-preview-settings-*.png' | Select-Object Name,Length
```

Expected: one non-empty full-board BIN, seven non-empty SETTINGS PNGs at 800x480, and a recorded SHA-256 checksum. Report the exact GitHub Actions URL, commit SHA, BIN path, checksum, and display the SETTINGS previews to the user.
