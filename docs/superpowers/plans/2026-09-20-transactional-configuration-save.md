# Transactional Configuration Save Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Make `BACK` reliably persist all Settings and tile changes, and make reset operations complete without hangs or LVGL artifacts.

**Architecture:** Introduce an explicit draft/commit boundary in the UI and serialize commit completion before navigation or screen reconstruction. Replace the tile parameter dropdown with the existing categorized picker and route tile exit, Settings exit, and resets through one completion-driven commit path.

**Tech Stack:** C++17, ESP32 Arduino Preferences/NVS, LVGL 8, PlatformIO Unity tests, Python source-contract tests, GitHub Actions.

**Spec:** `docs/superpowers/specs/2026-09-20-transactional-configuration-save.md`

## Global Constraints

- Run every test and firmware compilation only through GitHub Actions; do not compile or execute tests locally.
- Preserve the retained DASH/TRACK rendering strategy and do not perform NVS writes inside LVGL event callbacks.
- Do not commit `.superpowers/` or `artifacts/`.
- `BACK` is the user-facing save action; no separate Save button remains in Settings or the tile editor.

## Review Focus

- A second edit made while an older revision is in flight must remain dirty and must not be overwritten by stale completion.
- NVS write/erase failure must retain the draft and the current screen with controls usable for retry.
- Reset must not delete an overlay or rebuild a screen until the matching commit succeeds.
- Navigating from a Settings subpage to Settings home and then immediately to DASH must not drop the first pending snapshot.
- CAN and RaceChrono parameters must remain selectable after removal of the large dropdown.

---

### Task 1: Serialize configuration commits

**Files:**
- Modify: `src/ui/settings_commit_model.h`
- Modify: `src/ui/settings_commit_model.cpp`
- Modify: `test/test_settings_commit_model/test_main.cpp`

**Interfaces:**
- Produces: revision-aware `submit`, `take`, and `complete` behavior that retains a newer pending snapshot while an older request is in flight.
- Consumes: `ConfigCommitRequest`, `AppConfig`, and `ConfigCommitKind`.

- [ ] **Step 1: Write failing model tests**

Add Unity cases proving that an in-flight request cannot be taken twice, a newer submitted snapshot remains pending, stale success does not clear the newer draft, failure is retryable, and reset requests cannot overlap.

- [ ] **Step 2: Push the tests and verify RED on GitHub Actions**

Expected: the native test workflow fails in `test_settings_commit_model` because the current model does not represent an in-flight request or guarded reset.

- [ ] **Step 3: Implement the minimal commit state machine**

Track pending and in-flight revisions separately. `take()` moves exactly one request to in-flight state. `complete(revision, success)` accepts only the matching in-flight revision and leaves any newer pending request intact. Expose whether a commit is busy so UI controls can reject repeated reset/navigation actions.

- [ ] **Step 4: Push and verify GREEN on GitHub Actions**

Expected: all `test_settings_commit_model` cases pass.

- [ ] **Step 5: Commit**

Commit message: `fix: serialize configuration commits`

### Task 2: Save Settings on BACK after successful persistence

**Files:**
- Modify: `src/ui/ui.h`
- Modify: `src/ui/ui.cpp`
- Modify: `src/app/app.cpp`
- Modify: `scripts/tests/test_deferred_settings_save.py`
- Modify: `design/production-preview/transitions.cpp`

**Interfaces:**
- Consumes: the serialized commit model from Task 1.
- Produces: a UI draft, a pending post-commit navigation action, and success/failure completion handling.

- [ ] **Step 1: Write failing Settings transition tests**

Add source-contract and production LVGL transition coverage for changing Shift Light, pressing `BACK`, remaining on the page while the commit is outstanding, then navigating only after success. Add failure coverage that keeps the page and edited value available for retry.

- [ ] **Step 2: Push the tests and verify RED on GitHub Actions**

Expected: UI production-preview or script tests fail because current `settingsBackEvent` immediately rebuilds Settings home.

- [ ] **Step 3: Implement a separate Settings draft and deferred navigation**

Copy live configuration into a UI draft when Settings opens. Change Settings callbacks to mutate the draft. On `BACK`, submit the complete draft, disable navigation, display `SAVING`, and retain the screen. On matching success, update retained layouts once and navigate; on failure, show `SAVE ERROR` and re-enable `BACK` without discarding the draft.

- [ ] **Step 4: Push and verify GREEN on GitHub Actions**

Expected: native, source-contract, and production LVGL transition workflows pass.

- [ ] **Step 5: Commit**

Commit message: `fix: save settings transactionally on back`

### Task 3: Make resets completion-driven

**Files:**
- Modify: `src/ui/ui.h`
- Modify: `src/ui/ui.cpp`
- Modify: `src/ui/settings_commit_model.h`
- Modify: `src/ui/settings_commit_model.cpp`
- Modify: `design/production-preview/transitions.cpp`
- Modify: `scripts/tests/test_deferred_settings_save.py`

**Interfaces:**
- Consumes: the UI draft and serialized commit completion from Tasks 1-2.
- Produces: reset requests with explicit DASH, TRACK, or factory post-commit actions.

- [ ] **Step 1: Write failing reset transition tests**

Cover reset confirmation, unchanged screen before completion, one controlled screen rebuild after success, retained confirmation/draft after failure, and rejection of a second reset while one is running.

- [ ] **Step 2: Push the tests and verify RED on GitHub Actions**

Expected: production LVGL transition tests fail because the current reset handler closes/rebuilds UI before persistence completes.

- [ ] **Step 3: Implement completion-driven resets**

Keep the confirmation screen valid while submitting the reset. Disable confirmation controls during the operation. Apply defaults or reset layouts only through the committed candidate, close the overlay after success, and rebuild DASH/TRACK/Settings once. Restore controls and keep state on failure.

- [ ] **Step 4: Push and verify GREEN on GitHub Actions**

Expected: all reset transition and existing configuration repository tests pass.

- [ ] **Step 5: Commit**

Commit message: `fix: complete resets before rebuilding ui`

### Task 4: Replace tile SAVE and parameter dropdown with BACK and picker

**Files:**
- Modify: `src/ui/ui.h`
- Modify: `src/ui/ui.cpp`
- Modify: `src/ui/ui_tile_editor.cpp`
- Modify: `src/ui/ui_numeric_entry.cpp`
- Modify: `scripts/tests/test_tile_editor_screen.py`
- Modify: `design/production-preview/transitions.cpp`

**Interfaces:**
- Consumes: completion-driven commit/navigation from Tasks 1-2 and the existing categorized parameter picker.
- Produces: a lightweight current-parameter button and editor `BACK` commit behavior.

- [ ] **Step 1: Write failing tile-editor tests**

Require removal of the large parameter dropdown and `SAVE TILE`, opening the categorized picker from the current-parameter button, selecting one CAN and one RaceChrono parameter, pressing `BACK`, waiting for commit completion, and restoring the saved tile after reopening.

- [ ] **Step 2: Push the tests and verify RED on GitHub Actions**

Expected: source-contract and production LVGL transition tests fail on the existing dropdown and `SAVE TILE` behavior.

- [ ] **Step 3: Implement picker-only selection and BACK commit**

Display the current parameter in a button that opens `openParameterPicker()`. Remove dropdown synchronization and its event action. Rename the editor exit action to `BACK`; submit the complete tile candidate, retain the editor while saving, then close only on matching success. Preserve the draft and re-enable `BACK` after failure.

- [ ] **Step 4: Push and verify GREEN on GitHub Actions**

Expected: CAN and RaceChrono selection, persistence, failure retry, and existing tile editor tests all pass.

- [ ] **Step 5: Commit**

Commit message: `fix: save tile configuration on back`

### Task 5: Full GitHub verification and firmware artifact

**Files:**
- Modify only if a failing regression test identifies a production defect.
- Artifact: `DIY-Dash-ESP32-S3-Touch-LCD-5-full.bin`

**Interfaces:**
- Consumes: completed Tasks 1-4.
- Produces: verified GitHub workflow links, firmware size and SHA-256, and the downloadable full-flash binary.

- [ ] **Step 1: Push the integrated branch**

Trigger the complete native/script test workflow, production LVGL preview workflow, and firmware build workflow.

- [ ] **Step 2: Verify all workflows are GREEN**

Expected: all native tests, script tests, LVGL transitions, and ESP32-S3 firmware compilation pass on GitHub Actions.

- [ ] **Step 3: Download and identify the firmware artifact**

Download the full-flash BIN from the successful workflow, record its byte size and SHA-256, and keep it in the untracked `artifacts/` directory.

- [ ] **Step 4: Perform final code review**

Review the whole branch against the spec, paying particular attention to stale revisions, failure recovery, LVGL object lifetime, and reset re-entry.

- [ ] **Step 5: Commit any review fixes through RED-GREEN cycles**

If review finds an Important or Critical defect, add a failing GitHub test, implement the correction, and rerun all workflows before delivery.

