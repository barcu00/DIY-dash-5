# Dedicated Configuration Storage and RaceChrono UI Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Move DIY Dash configuration into a verified 512 KiB dedicated NVS partition, make failed saves recoverable from every exit path, and make the RaceChrono connection screen clear and easy to operate.

**Architecture:** A custom 16 MiB partition table carves `dashcfg` from the unused SPIFFS region without moving either OTA application slot. `NvsConfigBackend` targets that partition, while `ConfigRepository` verifies every write by reading the complete candidate back before changing live state. The existing deferred commit model remains the single save path for Settings and tile editing; UI failure state adds an explicit discard exit, and RaceChrono receives a large switch plus separate BLE, data, and GPS statuses.

**Tech Stack:** ESP32-S3, Arduino ESP32 3.1.1, ESP-IDF NVS through `Preferences`, PlatformIO 6.1.18, C++17, LVGL 8.4, Python 3.12 workflow tests, GitHub Actions.

**Spec:** `docs/superpowers/specs/2026-09-20-dedicated-config-storage-racechrono-ui-design.md`

## Global Constraints

- All automated tests and firmware compilation run on GitHub Actions; do not run PlatformIO, native tests, LVGL builds, or firmware compilation locally.
- The target remains `waveshare_esp32_s3_touch_lcd_5` with 16 MiB flash.
- `dashcfg` is an NVS partition at `0xC90000` with exact size `0x80000` (512 KiB).
- `app0`, `app1`, `otadata`, default `nvs`, and `coredump` offsets and sizes remain unchanged.
- Repository release assets remain limited to `DIY-Dash-ESP32-S3-Touch-LCD-5-full.bin`.
- Installing the new layout requires flashing the merged full binary at `0x0` and clears previously saved settings.
- BACK, DASH, and TRACK use the same verified commit path.
- No save failure may leave an input blocker active or permanently trap the user.
- Preserve the existing English-only UI.

## Review Focus

- A user flashes only `firmware.bin` over the old partition table: documentation and artifact naming must prevent presenting this as supported; Task 5 pins the full-image-only contract.
- NVS reports a successful write but read-back differs: live configuration must remain unchanged; Task 2 adds a mismatch test.
- A direct DASH or TRACK exit fails and the user discards: the originally requested destination must be preserved and preview brightness restored; Task 3 tests both destinations.
- A touch lands on the RaceChrono switch inside the tappable row: exactly one state transition must occur; Task 4 tests switch and row events independently.
- RaceChrono is disabled after previously receiving GPS data: stale `ACTIVE` or fix text must not remain visible; Task 4 tests disabled, waiting, connected, and active states.

---

### Task 1: Custom partition layout and binary validation

**Files:**
- Create: `partitions_diy_dash.csv`
- Create: `scripts/validate_partition_bin.py`
- Create: `scripts/tests/test_partition_layout.py`
- Modify: `platformio.ini`
- Modify: `.github/workflows/build-firmware.yml`

**Interfaces:**
- Consumes: the existing PlatformIO `waveshare_5` environment and generated `.pio/build/waveshare_5/partitions.bin`.
- Produces: `parse_partition_table(data: bytes) -> dict[str, tuple[int, int, int, int]]` in `scripts/validate_partition_bin.py`, plus the firmware partition named `dashcfg` at offset `0xC90000`, size `0x80000`.

- [ ] **Step 1: Write the failing partition contract tests**

Create `scripts/tests/test_partition_layout.py` with tests that read the CSV and PlatformIO configuration:

```python
import configparser
import csv
import pathlib
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[2]


class PartitionLayoutTests(unittest.TestCase):
    def test_dedicated_configuration_partition_has_future_capacity(self):
        with (ROOT / "partitions_diy_dash.csv").open(newline="") as source:
            rows = {row[0]: row for row in csv.reader(
                line for line in source if line.strip() and not line.startswith("#")
            )}
        self.assertEqual("0xC90000", rows["dashcfg"][3])
        self.assertEqual("0x80000", rows["dashcfg"][4])
        self.assertEqual("nvs", rows["dashcfg"][2])
        self.assertEqual("0xD10000", rows["spiffs"][3])
        self.assertEqual("0x2E0000", rows["spiffs"][4])

    def test_platformio_selects_the_custom_table(self):
        text = (ROOT / "platformio.ini").read_text(encoding="utf-8")
        waveshare = text.split("[env:waveshare_5]", 1)[1]
        self.assertIn("board_build.partitions = partitions_diy_dash.csv", waveshare)
```

Add parser tests using packed 32-byte ESP partition entries and assert that an entry with label `dashcfg`, data type, NVS subtype, offset `0xC90000`, and size `0x80000` is accepted, while a 20 KiB `dashcfg` entry raises `ValueError`.

- [ ] **Step 2: Push the RED tests and verify GitHub failure**

```bash
git add scripts/tests/test_partition_layout.py
git commit -m "test: require dedicated dashboard configuration partition"
git push origin feature/racechrono-ble-monitor
```

Expected GitHub result: `DIY Dash firmware build / unit-tests` fails because `partitions_diy_dash.csv` and the parser do not exist. Record the run URL in the SDD ledger.

- [ ] **Step 3: Implement the exact partition table**

Create `partitions_diy_dash.csv`:

```csv
# Name,     Type, SubType, Offset,   Size,     Flags
nvs,        data, nvs,     0x9000,   0x5000,
otadata,    data, ota,     0xE000,   0x2000,
app0,       app,  ota_0,   0x10000,  0x640000,
app1,       app,  ota_1,   0x650000, 0x640000,
dashcfg,    data, nvs,     0xC90000, 0x80000,
spiffs,     data, spiffs,  0xD10000, 0x2E0000,
coredump,   data, coredump,0xFF0000, 0x10000,
```

Add this to `[env:waveshare_5]` in `platformio.ini`:

```ini
board_build.partitions = partitions_diy_dash.csv
```

- [ ] **Step 4: Implement generated-binary validation**

In `scripts/validate_partition_bin.py`, unpack entries with `struct.Struct("<HBBII16sI")`, stop at erased or MD5 records, and reject any table where:

```python
EXPECTED = {
    "nvs": (0x01, 0x02, 0x009000, 0x005000),
    "otadata": (0x01, 0x00, 0x00E000, 0x002000),
    "app0": (0x00, 0x10, 0x010000, 0x640000),
    "app1": (0x00, 0x11, 0x650000, 0x640000),
    "dashcfg": (0x01, 0x02, 0xC90000, 0x080000),
    "spiffs": (0x01, 0x82, 0xD10000, 0x2E0000),
    "coredump": (0x01, 0x03, 0xFF0000, 0x010000),
}
```

Export `parse_partition_table`. The script entry point accepts one path, compares the parsed table with `EXPECTED`, prints the verified `dashcfg` offset and size, and exits nonzero on a mismatch.

Add after the firmware build in `.github/workflows/build-firmware.yml`:

```yaml
      - name: Verify firmware partition table
        run: python scripts/validate_partition_bin.py .pio/build/waveshare_5/partitions.bin
```

- [ ] **Step 5: Push GREEN and verify both tests and firmware packaging**

```bash
git add partitions_diy_dash.csv platformio.ini scripts/validate_partition_bin.py scripts/tests/test_partition_layout.py .github/workflows/build-firmware.yml
git commit -m "build: add dedicated dashboard configuration partition"
git push origin feature/racechrono-ble-monitor
```

Expected: unit tests pass, the Waveshare build passes, and the workflow prints `dashcfg offset=0xC90000 size=0x80000` before uploading the firmware artifact.

### Task 2: Verified persistence and isolated NVS backend

**Files:**
- Modify: `src/settings/nvs_config_backend.h`
- Modify: `src/settings/nvs_config_backend.cpp`
- Modify: `src/settings/config_repository.cpp`
- Modify: `test/test_config_repository/test_main.cpp`
- Create: `scripts/tests/test_nvs_partition_contract.py`

**Interfaces:**
- Consumes: `ConfigBackend::{storedSize,read,write,erase}` and `AppConfig::validate()`.
- Produces: `NvsConfigBackend::kPartition = "dashcfg"`; `ConfigRepository::saveCandidate()` returns true only after byte-identical read-back and changes `runtime_config` only on verified success.

- [ ] **Step 1: Add failing repository verification tests**

Extend `MemoryBackend` in `test/test_config_repository/test_main.cpp` with `fail_reads` and `corrupt_after_write` controls. Add:

```cpp
bool read(void* data, std::size_t size) override {
    if (fail_reads || !has_value || size != stored_size) return false;
    std::memcpy(data, bytes.data(), size);
    if (corrupt_after_write && size > 0U)
        static_cast<uint8_t*>(data)[size - 1U] ^= 0x01U;
    return true;
}

bool fail_reads = false;
bool corrupt_after_write = false;
```

Keep the existing `fail_writes` path as the backend-open/short-write failure
contract: in either hardware case `NvsConfigBackend::write()` returns false,
and the repository must preserve the prior runtime configuration. Add:

```cpp
void test_save_requires_readback_before_applying_runtime() {
    MemoryBackend backend;
    backend.fail_reads = true;
    ConfigRepository repository(backend);
    AppConfig runtime = AppConfig::defaults();
    AppConfig candidate = runtime;
    candidate.brightness_percent = 40U;

    TEST_ASSERT_FALSE(repository.saveCandidate(candidate, runtime));
    TEST_ASSERT_EQUAL_UINT8(100U, runtime.brightness_percent);
}

void test_save_rejects_mismatched_readback() {
    MemoryBackend backend;
    backend.corrupt_after_write = true;
    ConfigRepository repository(backend);
    AppConfig runtime = AppConfig::defaults();
    AppConfig candidate = runtime;
    candidate.racechrono.enabled = true;

    TEST_ASSERT_FALSE(repository.saveCandidate(candidate, runtime));
    TEST_ASSERT_FALSE(runtime.racechrono.enabled);
}
```

Register both with `RUN_TEST`.

- [ ] **Step 2: Add a failing dedicated-partition source contract**

Create `scripts/tests/test_nvs_partition_contract.py` and assert:

```python
header = (ROOT / "src/settings/nvs_config_backend.h").read_text()
source = (ROOT / "src/settings/nvs_config_backend.cpp").read_text()
self.assertIn('kPartition = "dashcfg"', header)
self.assertGreaterEqual(source.count("kPartition"), 4)
self.assertIn("[CONFIG] NVS open failed", source)
self.assertIn("[CONFIG] NVS write failed", source)
self.assertIn("[CONFIG] NVS read failed", source)
```

- [ ] **Step 3: Push RED and verify the expected failures on GitHub**

Commit the tests as `test: require verified isolated configuration storage`, push, and confirm repository verification plus source-contract failures.

- [ ] **Step 4: Implement read-back verification in the repository**

Change `ConfigRepository::saveCandidate` to preserve active state until verification completes:

```cpp
AppConfig validated = candidate;
if (validated.schema_version != AppConfig::kSchemaVersion ||
    !validated.validate().valid ||
    !backend_.write(&validated, sizeof(validated))) {
    return false;
}

AppConfig stored{};
if (backend_.storedSize() != sizeof(stored) ||
    !backend_.read(&stored, sizeof(stored)) ||
    std::memcmp(&stored, &validated, sizeof(stored)) != 0) {
    return false;
}

runtime_config = stored;
return true;
```

Include `<cstring>` explicitly. Ensure layout resets continue to call this method.

- [ ] **Step 5: Point all Preferences operations at `dashcfg` and log boundaries**

Add to `NvsConfigBackend`:

```cpp
static constexpr const char* kPartition = "dashcfg";
```

Open every handle with the three-argument overload:

```cpp
preferences.begin(kNamespace, read_only, kPartition)
```

For each failed open, short write, invalid read length, and clear failure, print one concise serial message with operation and expected/actual byte counts. Do not print raw configuration bytes. `erase()` continues to call `clear()` only inside namespace `diy_dash` on `dashcfg`.

- [ ] **Step 6: Push GREEN and verify full GitHub build**

Commit as `fix: isolate and verify configuration persistence`. Verify native tests, Python contracts, and the Waveshare compilation. A compile failure on the `Preferences.begin` overload must be fixed against the pinned Arduino 3.1.1 API rather than bypassing the named partition.

### Task 3: Recoverable save failures for Settings and tile editor

**Files:**
- Modify: `src/ui/ui.h`
- Modify: `src/ui/ui.cpp`
- Modify: `src/ui/ui_tile_editor.cpp`
- Modify: `design/production-preview/transitions.cpp`
- Modify: `.github/workflows/production-view-previews.yml`
- Modify: `scripts/tests/test_deferred_settings_save.py`
- Modify: `scripts/tests/test_tile_editor_screen.py`

**Interfaces:**
- Consumes: existing `requestSettingsExit(Page)`, `requestSettingsExit(SettingsCategory)`, `completeConfigCommit(revision, success)`, and editor commit state.
- Produces: `discardFailedSettingsExit()`, `discardFailedEditorExit()`, and a visible `EXIT WITHOUT SAVE` action only after a failed commit.

- [ ] **Step 1: Add failing transition tests for BACK, DASH, and TRACK**

Extend the production transition executable with `--save-exits`. For each destination:

1. Enter Settings and change brightness.
2. Click BACK, DASH, or TRACK.
3. Take the queued commit and complete it with `false`.
4. Assert the original settings screen is still active, the blocker is gone, and `EXIT WITHOUT SAVE` exists.
5. Click `EXIT WITHOUT SAVE`.
6. Assert the requested destination is active, `config.brightness_percent` is unchanged, and the board brightness preview returned to the active value.

Add the workflow step:

```yaml
      - name: Check recoverable save failures for every exit
        if: success() || failure()
        run: /tmp/ui-transitions --save-exits
```

- [ ] **Step 2: Add failing tile-editor retry/discard assertions**

In the existing `--retry` scenario, after a failed completion assert that BACK retries the same candidate and CANCEL/`EXIT WITHOUT SAVE` closes without changing `config`. Add static Python assertions that a failed save never calls `closeEditor()` and restores both action buttons.

- [ ] **Step 3: Push RED and capture both LVGL and firmware workflow failures**

Commit as `test: require recoverable configuration save failures`, push, and confirm the new transition scenarios fail because the discard action is absent.

- [ ] **Step 4: Implement Settings failure recovery**

Add these fields and methods to `Ui`:

```cpp
lv_obj_t* settings_discard_exit_ = nullptr;
bool settings_save_failed_ = false;
void showSettingsSaveFailure();
void discardFailedSettingsExit();
static void discardSettingsExitEvent(lv_event_t* event);
```

On Settings commit failure:

- clear `pending_settings_exit_` only after preserving its destination fields;
- delete `settings_commit_blocker_`;
- restore live brightness;
- set `settings_save_failed_`;
- show `SAVE ERROR` and a top-layer `EXIT WITHOUT SAVE` button;
- leave BACK and navigation enabled so any of them queues a retry with its newly requested destination.

`discardFailedSettingsExit()` copies `*config_` back into `settings_draft_`, clears the commit model's dirty retry state through a narrowly named `SettingsCommitModel::discardChanges()` method, deletes the failure action, restores brightness, and performs the preserved destination transition without persisting.

Implement the model operation as:

```cpp
bool SettingsCommitModel::discardChanges() {
    if (busy()) return false;
    dirty_ = false;
    reconfigure_runtime_ = false;
    pending_request_ = ConfigCommitRequest{};
    return true;
}
```

Add native tests for `discardChanges()` proving it is rejected while a request is in flight and clears dirty state only after a failed request has completed.

- [ ] **Step 5: Implement tile-editor failure recovery**

After editor failure, keep BACK as retry, enable CANCEL, change its visible label to `EXIT WITHOUT SAVE`, and ensure the editor candidate is discarded by `closeEditor()` without touching `settings_draft_` or `config_`. A new successful retry restores the normal close path and removes all failure-only UI.

- [ ] **Step 6: Push GREEN and verify production framebuffer stability**

Commit as `fix: make save failures recoverable`. Verify the firmware workflow and production LVGL workflow, including partial/direct framebuffer comparisons, before continuing.

### Task 4: RaceChrono switch and unambiguous connection status

**Files:**
- Modify: `src/ui/ui.h`
- Modify: `src/ui/ui_racechrono.cpp`
- Modify: `design/production-preview/transitions.cpp`
- Modify: `scripts/ui_preview/render_ui_preview.py`
- Modify: `.github/workflows/production-view-previews.yml`
- Modify: `scripts/tests/test_racechrono_docs.py`

**Interfaces:**
- Consumes: `settings_draft_.racechrono.enabled`, `RaceChronoConnectionState`, and `RaceChronoRuntimeStatus`.
- Produces: a 64x32 `lv_switch`, a single row-toggle callback, and separate BLE/data/GPS labels.

- [ ] **Step 1: Add failing LVGL interaction and status tests**

Update `--racechrono` to assert:

```cpp
auto* enabled = find(lv_scr_act(), &lv_switch_class);
assert(enabled);
assert(lv_obj_get_width(enabled) >= 64);
assert(lv_obj_get_height(enabled) >= 32);
```

Send one click to the switch and assert one staged transition to enabled. Reopen with disabled configuration, click the ENABLED row outside the switch, and assert one transition. Then exercise these status snapshots and exact visible strings:

- disabled -> `BLE DISABLED`, no stale `DATA: ACTIVE`, no stale GPS fix;
- advertising/no client -> `BLE: WAITING FOR APP` and `GPS: NO FIX`;
- connected/no packet -> `BLE: CONNECTED` and `DATA: WAITING`;
- valid packet with fix type 3 -> `DATA: ACTIVE` and `GPS: 3D FIX`.

- [ ] **Step 2: Push RED and verify the RaceChrono LVGL run fails**

Commit as `test: define readable RaceChrono connection controls`, push, and verify the production LVGL job fails on the missing switch and status strings.

- [ ] **Step 3: Implement the large switch and row hit target**

Replace `lv_checkbox_create` with `lv_switch_create`, size it to 64x32, and position it with at least 16 pixels of right margin. Add a transparent clickable row object covering the ENABLED row except the switch bounds. Both event paths call one helper:

```cpp
void Ui::setRaceChronoDraftEnabled(bool enabled);
```

The switch's `LV_EVENT_VALUE_CHANGED` reads its checked state. The row's `LV_EVENT_CLICKED` toggles that state once and sends no second synthetic value-change event. Rebuilding the page must reflect `settingsConfig().racechrono.enabled`.

- [ ] **Step 4: Implement separate BLE, data, and GPS status labels**

Replace `racechrono_signal_` with:

```cpp
lv_obj_t* racechrono_ble_status_ = nullptr;
lv_obj_t* racechrono_data_status_ = nullptr;
lv_obj_t* racechrono_gps_status_ = nullptr;
```

Derive display text first from the staged enabled state, then connection state, then telemetry validity. Disabled state overrides stale runtime telemetry. Use red only for `RaceChronoConnectionState::Error`, green only for connected/active/fix states, and muted text for disabled or waiting.

- [ ] **Step 5: Produce and compare all RaceChrono screenshots**

Capture `racechrono-disabled`, `racechrono-waiting`, `racechrono-connected`, and `racechrono-active` in full and partial/direct modes. Extend the workflow comparison loop so every state must render identically through both paths.

- [ ] **Step 6: Push GREEN and inspect uploaded images**

Commit as `fix: clarify RaceChrono connection controls`. Verify both workflows, download the `RaceChrono-LVGL-previews` artifact, and visually inspect switch alignment, row spacing, status colors, and absence of overlap at 800x480.

### Task 5: Documentation, final verification, and test firmware

**Files:**
- Modify: `README.md`
- Modify: `docs/ui/dashboard-config-guide.md`
- Modify: `docs/racechrono/setup.md`
- Modify: `scripts/tests/test_racechrono_docs.py`
- Modify: `scripts/tests/test_merge_bin.py`

**Interfaces:**
- Consumes: the final partition table, save behavior, RaceChrono UI, and GitHub build artifact.
- Produces: user-facing full-image migration instructions and the final `DIY-Dash-ESP32-S3-Touch-LCD-5-full.bin` test image.

- [ ] **Step 1: Add failing documentation and artifact-policy tests**

Require README and setup documentation to contain all of:

```text
dashcfg
512 KiB
flash at 0x0
clears existing settings
DIY-Dash-ESP32-S3-Touch-LCD-5-full.bin
EXIT WITHOUT SAVE
BLE: WAITING FOR APP
GPS: NO FIX
```

Extend merge tests to assert the manifest still records bootloader at `0x0000`, partition table at `0x8000`, boot app at `0xe000`, and application at `0x10000`; this pins the full-image-only migration contract.

- [ ] **Step 2: Push RED and verify documentation contracts fail**

Commit as `test: require configuration storage migration guidance`, push, and record the failing firmware workflow.

- [ ] **Step 3: Update documentation**

Explain that the first build containing `dashcfg` must be installed using the merged full binary at `0x0`, that this installation clears previous settings, and that subsequent normal saves are isolated from BLE/system NVS. Document all save states, direct DASH/TRACK saving, retry/discard behavior, and the revised RaceChrono statuses and switch.

- [ ] **Step 4: Push GREEN and run final GitHub verification**

Commit as `docs: document dedicated configuration storage`, push, and require:

- `DIY Dash firmware build`: success;
- `Production LVGL view previews`: success;
- generated partition validation: success;
- uploaded `DIY-Dash-firmware` artifact: present;
- uploaded RaceChrono screenshots: present.

- [ ] **Step 5: Download and verify the exact full binary**

Download the artifact from the final successful firmware run, extract only the release candidate, and compute SHA-256 without executing it:

```powershell
Get-FileHash -Algorithm SHA256 -LiteralPath `
  artifacts/firmware-<commit>/DIY-Dash-ESP32-S3-Touch-LCD-5-full.bin
```

Verify its size is exactly 16,777,216 bytes and the artifact ZIP digest matches GitHub's displayed digest. Provide the absolute clickable path, commit SHA, workflow URLs, size, and SHA-256 to the user.

- [ ] **Step 6: Perform whole-branch review and handoff**

Review the complete diff from the pre-plan base with focus on partition overlap, live-state mutation before verification, stale commit completion, failure UI object lifetime, and switch double-toggle. Fix Critical or Important findings through new RED/GREEN GitHub cycles. Leave the branch pushed and ask the user to select merge, pull request, or branch preservation; do not merge or delete it without the user's choice.
