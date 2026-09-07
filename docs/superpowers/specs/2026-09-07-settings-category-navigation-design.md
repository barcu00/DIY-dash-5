# Settings Category Navigation Design

## Goal

Rebuild the SETTINGS area for consistently smooth operation on the Waveshare ESP32-S3-Touch-LCD-5 (800x480). Replace the current long scrolling page with small, fixed-size category screens while preserving every existing setting and the existing tile editor behavior.

## Scope

This change covers the SETTINGS landing screen, category navigation, settings persistence, layout-slot navigation, and SETTINGS-specific update behavior. It does not change DASH or TRACK geometry, CAN decoding, warning presentation, or the visual design of the existing tile editor.

## Navigation Architecture

SETTINGS opens on a landing screen containing six category buttons:

1. DISPLAY
2. DATA & CAN
3. SHIFT LIGHT
4. UNITS
5. LAYOUTS
6. SYSTEM

Selecting a category replaces the landing screen with a fixed, non-scrollable category screen. A BACK control returns to the landing screen. The shared bottom navigation continues to provide DASH, TRACK, and SETTINGS access.

Only the currently visible SETTINGS screen is instantiated. When navigation completes, objects owned by the previous SETTINGS screen are deleted. Common navigation state is retained separately from category widgets. This prevents inactive settings controls from participating in LVGL layout, invalidation, input processing, or drawing.

## Category Contents

### DISPLAY

- Brightness adjustment from the currently supported minimum to maximum.
- Runtime display information.
- Available runtime memory information when the platform exposes it.
- Display resolution, pixel format, and configured LVGL buffer size.

### DATA & CAN

- Explicit source selection: CAN or DEMO.
- CAN profile selection from the profiles compiled into the firmware.
- CAN bitrate.
- CAN timeout.
- Current connection and receive status.

The status field updates independently without rebuilding or relaying out the complete screen. Nonessential status rendering is suspended while a control is actively manipulated.

### SHIFT LIGHT

- Shift-light start RPM.
- Red-zone RPM.
- Maximum RPM.

The firmware maintains the invariant:

`start RPM < red-zone RPM < maximum RPM`

If changing one value would violate this order, dependent values are adjusted to the nearest valid supported values. The corrected values are immediately reflected in the controls and persisted.

### UNITS

- Temperature units.
- Pressure units.
- Speed units.
- Any additional unit families already supported by the application data model.

The screen must not introduce settings for units that the runtime cannot currently format.

### LAYOUTS

The first control selects DASH or TRACK. Slots are then displayed in pages of no more than six entries, so the screen never requires vertical scrolling and never creates controls for the entire layout at once.

Each slot entry shows its location, selected parameter, and visible/hidden state. Selecting an entry opens the existing tile editor. Parameter, visibility, and warning configuration remain fully configurable. Closing the editor automatically persists that individual tile configuration and refreshes the affected slot entry.

Pagination uses explicit PREVIOUS and NEXT controls plus a current-page indicator. Controls that cannot move to another page are disabled.

### SYSTEM

- Firmware version and build information.
- Runtime internal-memory and PSRAM information when available.
- Reset DASH layout.
- Reset TRACK layout.
- Factory reset.

All reset operations require confirmation before modifying stored settings. Resetting one layout does not alter the other layout or unrelated settings. Factory reset restores every persisted application setting covered by the existing factory-reset behavior.

## Persistence Behavior

There is no SAVE button.

- Switches, segmented choices, and selection lists are persisted immediately after a valid selection change.
- Sliders update their displayed value continuously while dragged, but NVS is written only when the user releases the control.
- Tile editor values are persisted when the editor closes after a change.
- A successful write displays a short, non-modal `SAVED` message.
- A failed write displays a non-modal `SAVE ERROR` message and keeps the selected value visible so the user can retry or change it.

Avoiding NVS writes for every slider movement protects flash endurance and prevents input/rendering stalls.

## Rendering and Input Rules

- Category screens fit between the header and bottom navigation at 800x480 without scrolling.
- No long SETTINGS scroll container is created.
- Only the active category owns live controls and periodic display updates.
- Touch handling must not synchronously rebuild the category screen.
- Navigation creates the destination screen, switches to it, and then releases the source screen without leaving dangling callbacks.
- Existing 80-line double buffering remains unchanged unless GitHub verification identifies a separate rendering defect.
- CAN processing, alarm evaluation, and other safety-relevant logic continue while SETTINGS is open; only nonessential visual refreshes may be deferred during touch interaction.

## Error Handling

- Invalid Shift Light combinations are corrected before persistence.
- Unsupported CAN or unit selections are not presented.
- NVS failures are reported through `SAVE ERROR` without opening a modal or blocking navigation.
- Reset confirmation can be cancelled without modifying settings.
- If a stored layout slot cannot be represented by the current firmware, the slot displays a safe fallback parameter while preserving the rest of the layout.

## Test Strategy

All builds and tests run in GitHub Actions; no firmware build or test is run in the local Codex environment.

Automated tests must cover:

- SETTINGS landing categories and category-to-landing navigation.
- At most one active SETTINGS category screen at a time.
- Fixed, non-scrollable category layouts.
- Immediate persistence for discrete controls.
- Persistence on slider release rather than during slider movement.
- Shift Light ordering and automatic correction.
- DASH/TRACK selection and six-entry pagination in LAYOUTS.
- Tile editor reuse and per-tile persistence.
- Reset confirmation and reset scope.
- Non-blocking saved/error feedback.
- Suspension of nonessential status redraws during control interaction.

The implementation follows a RED/GREEN sequence in GitHub Actions: first add tests that fail against the current long SETTINGS page, then implement the category architecture and require the complete workflow to pass.

After a successful GitHub Actions run, download the firmware artifact, record the workflow URL and SHA-256 checksum for the full-board BIN, and present UI previews generated by that same build.

## Acceptance Criteria

- SETTINGS contains no vertically scrolling mega-page.
- Every existing user-facing setting remains reachable through one of the six categories.
- Each category fits on the target 800x480 display without scrolling.
- Changing a discrete setting persists it immediately; moving a slider persists it on release.
- Layout editing remains fully configurable and persists each edited tile.
- Navigation and control interaction remain responsive while CAN and alarm processing continue.
- The complete GitHub Actions test and firmware build workflow passes and produces a flashable full-board BIN plus SETTINGS previews.
