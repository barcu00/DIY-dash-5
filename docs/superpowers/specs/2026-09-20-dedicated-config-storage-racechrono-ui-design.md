# Dedicated configuration storage and RaceChrono UI design

## Goal

Make configuration saves reliable as the dashboard grows, ensure that leaving
Settings through BACK, DASH, or TRACK uses the same save transaction, and make
the RaceChrono connection screen easier to understand and operate.

Success means that repeated configuration writes do not compete with ESP32 or
BLE system data, a failed write never traps the user permanently, and the
RaceChrono enable control has a large, reliable touch target.

## Confirmed root cause

The persisted `AppConfig` is currently 7,380 bytes. The released partition
table provides only 20 KiB for the shared default NVS partition. Updating an
NVS blob requires free entries for the new value before the previous value can
be reclaimed. System and BLE entries use the same partition, so a valid
configuration can pass application validation while `Preferences.putBytes()`
fails. The UI then correctly reports `SAVE ERROR` but cannot complete the
requested transition.

The existing navigation callbacks are not the source of the failure. BACK and
direct navigation to DASH or TRACK already enter the deferred commit flow.

## Flash partition layout

Add a custom partition table derived from the current 16 MiB layout. Keep the
bootloader, partition-table location, OTA metadata, both application slots,
and coredump region unchanged. Carve a dedicated NVS partition out of the
currently unused SPIFFS region.

| Partition | Offset | Purpose | Size |
| --- | ---: | --- | ---: |
| `nvs` | `0x009000` | ESP32 framework and BLE data | 20 KiB, unchanged |
| `otadata` | `0x00E000` | OTA selection metadata | 8 KiB, unchanged |
| `app0` | `0x010000` | First application slot | 6.25 MiB, unchanged |
| `app1` | `0x650000` | Second application slot | 6.25 MiB, unchanged |
| `dashcfg` | `0xC90000` | DIY Dash configuration only | 512 KiB |
| `spiffs` | `0xD10000` | Reserved filesystem space | 2.875 MiB |
| `coredump` | `0xFF0000` | Crash data | 64 KiB, unchanged |

The dedicated partition is intentionally much larger than the current object.
It provides room for additional layouts, vehicle profiles, analog-input
calibration, RaceChrono settings, and future per-car configuration without
another partition-table migration.

The PlatformIO Waveshare target must explicitly select this partition CSV. The
firmware packaging test must decode or otherwise inspect `partitions.bin` and
verify the `dashcfg` label and exact 512 KiB size.

## Configuration backend

`NvsConfigBackend` will open the `diy_dash` namespace in the `dashcfg`
partition rather than the shared default NVS partition. The repository keeps
one versioned `AppConfig` snapshot, preserving the existing atomic candidate
model and schema migration code.

Each save will:

1. Validate the candidate without modifying the active runtime configuration.
2. Write the complete candidate to `dashcfg`.
3. Read the value back into a verification buffer.
4. Compare the stored bytes with the validated candidate.
5. Update the live configuration only after write and verification succeed.

Failures must report which boundary failed in the serial log: partition open,
write length, read-back length, or verification mismatch. Logs must include the
configuration size but must not print raw configuration contents.

Factory reset clears only the `diy_dash` namespace in `dashcfg`. It must not
erase the framework/BLE NVS partition.

## Save and navigation behavior

BACK, DASH, and TRACK will all call the same deferred save request when the
Settings draft is dirty. The current screen remains intact while persistence
is in progress. A successful verified commit applies runtime changes and then
performs the requested navigation.

While a commit is active, duplicate navigation and save inputs are blocked.
The UI exposes these states consistently:

- `UNSAVED` after a draft change;
- `SAVING` while the backend owns the snapshot;
- `SAVED` after verified persistence;
- `SAVE ERROR` after any backend failure.

On `SAVE ERROR`, the transparent blocker is removed and controls are restored.
The user can retry the original navigation or choose `EXIT WITHOUT SAVE`.
Exiting without saving discards the Settings draft, restores preview-only
effects such as brightness to the active configuration, and navigates to the
chosen destination. It never reports `SAVED` and never mutates the active
configuration.

The tile editor uses the same transaction rules. Its BACK button saves and
closes only after verification. A failed tile save keeps the editor open and
offers retry or exit without saving.

## RaceChrono connection screen

Replace the small empty-text checkbox with a switch whose visible and touch
area is at least 64 by 32 pixels. The complete ENABLED row is also tappable and
toggles the same staged setting. The row and switch must not generate duplicate
toggle events from one touch.

Separate transport and GPS information:

- when disabled: `BLE DISABLED`;
- enabled but not connected: `BLE: WAITING FOR APP`;
- connected without data: `BLE: CONNECTED` and `DATA: WAITING`;
- receiving data: `DATA: ACTIVE`;
- GPS state: `GPS: NO FIX`, `GPS: 2D FIX`, or `GPS: 3D FIX`.

`NO FIX` is a GPS state, not a BLE error, and must not share one ambiguous
message with the BLE waiting state. Genuine transport errors remain red;
waiting and disabled states use neutral text; active data uses green.

## Flashing and migration

The new partition table requires one full-chip image installation. The release
artifact remains `DIY-Dash-ESP32-S3-Touch-LCD-5-full.bin`, flashed at address
`0x0`. This first full-image installation clears existing saved settings. The
README and release notes must state this explicitly.

No compatibility promise is made for updating only `firmware.bin` from a build
that uses the old partition table. Repository releases continue to contain only
the merged full binary, matching the established release policy.

## Verification

All compilation and automated testing run on GitHub Actions, never in the local
workspace.

Required automated coverage:

- native repository tests for successful write/read verification;
- failure tests for open, short write, failed read-back, and mismatched data;
- factory reset test proving that only the dashboard namespace is erased;
- commit-flow tests for BACK, DASH, TRACK, retry, and exit without saving;
- tile-editor failure and retry tests;
- production LVGL tests for the enlarged RaceChrono switch and full-row toggle;
- production LVGL screenshots for disabled, waiting, connected/no-data, and
  active/GPS-fix states;
- partition artifact validation for `dashcfg` at exactly 512 KiB;
- full Waveshare firmware build and merged full-binary generation.

Physical acceptance on the board must repeat saves across Display, Shift
Light, tile editor, CAN, and RaceChrono settings, then power-cycle and confirm
that values reload. It must also verify direct Settings-to-DASH and
Settings-to-TRACK saves and repeated RaceChrono enable/disable operations.
