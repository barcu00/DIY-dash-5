# Transactional Configuration Save Design

## Goal

Make configuration persistence reliable across Settings, tile editing, and reset operations without blocking LVGL or rebuilding screens while a storage operation is incomplete.

## Approved interaction

- Settings pages have no separate Save action.
- `BACK` saves every pending setting before leaving the current settings screen.
- The tile editor uses `BACK` as its commit-and-exit action instead of `SAVE TILE`.
- While a commit is running, the relevant navigation controls are disabled and the current screen remains intact.
- A successful commit changes the screen once and shows `SAVED`.
- A failed commit keeps the editor/settings screen open, preserves the draft, restores controls, and shows `SAVE ERROR`.
- Reset DASH, reset TRACK, and factory reset use the same completion-driven transition and never rebuild the UI before storage reports success.

## Architecture

The UI owns an editable configuration draft separate from the live `AppConfig` used by the runtime. Widget callbacks mutate only the draft and mark it dirty. `BACK` submits one immutable snapshot through `SettingsCommitModel`; `App` persists that snapshot outside the LVGL callback and reports completion while holding the display lock. Only successful completion updates/reconfigures the retained DASH and TRACK views and performs the requested navigation.

The commit model serializes requests. A request already taken by `App` is considered in flight, and a newer draft cannot be cleared or overwritten by completion of an older revision. Reset is represented as an explicit commit request with a completion action rather than immediately deleting widgets or mutating the runtime configuration.

## Parameter selection

The large parameter dropdown is removed from the tile editor. The current parameter is displayed in a lightweight button; pressing it opens the existing full-screen categorized picker for Demo, CAN, flags, and RaceChrono parameters. Returning from the picker changes the tile draft. Pressing the editor `BACK` persists the chosen parameter and exits only after successful storage completion.

## Verification

All compilation and automated tests run only on GitHub Actions. Tests cover:

- Shift-light edits saved by `BACK` and restored from the persisted configuration.
- A CAN parameter selected for a tile, committed with `BACK`, and restored.
- A RaceChrono parameter selected for a tile, committed with `BACK`, and restored.
- A failed storage operation leaving the current screen and draft intact.
- A newer edit surviving completion of an older commit revision.
- DASH reset, TRACK reset, and factory reset changing screens only after successful completion.
- Repeated reset requests being ignored while a reset is in flight.

