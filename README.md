# DIY Dash 5

[![Firmware build](https://github.com/barcu00/DIY-dash-5/actions/workflows/build-firmware.yml/badge.svg?branch=main)](https://github.com/barcu00/DIY-dash-5/actions/workflows/build-firmware.yml)
[![Current release](https://img.shields.io/github/v/release/barcu00/DIY-dash-5?include_prereleases&label=current%20firmware)](https://github.com/barcu00/DIY-dash-5/releases/tag/v0.2.1)
[![Target](https://img.shields.io/badge/target-ESP32--S3-00979D)](https://www.espressif.com/en/products/socs/esp32-s3)

DIY Dash 5 is an open, configurable auxiliary CAN dashboard for motorsport and
road cars. It is designed for the **Waveshare ESP32-S3-Touch-LCD-5 non-B**
board with an 800 × 480 RGB display, GT911 touch controller, 16 MB flash, and
8 MB OPI PSRAM.

The dashboard does not replace the factory instrument cluster. It adds a
dedicated screen for engine, ECU, and vehicle data, configurable warnings,
temperature status, and a track-oriented shift light. CAN operation is passive
and receive-only.

## Current firmware

The current test-board release is **v0.2.1**.

- [Open the v0.2.1 release](https://github.com/barcu00/DIY-dash-5/releases/tag/v0.2.1)
- [Download the complete 16 MB flash image](https://github.com/barcu00/DIY-dash-5/releases/download/v0.2.1/DIY-Dash-ESP32-S3-Touch-LCD-5-full.bin)
- [View the validated GitHub Actions build](https://github.com/barcu00/DIY-dash-5/actions/runs/34399159652)

Flash the complete image at address **`0x0`**. Its expected SHA-256 is:

```text
9FC9DD483754B7DDDF8EE3D20B1D87F31489FC43DA1D19DF0F165997BE9AAF70
```

> [!IMPORTANT]
> v0.2.1 is a prerelease for a physical test board. Automated tests verify the
> software, UI previews, ESP32-S3 compilation, and binary packaging. Vehicle
> wiring, touch response, CAN traffic, temperature, and long-duration stability
> still require validation on the actual installation.

## Interface preview

| DASH | TRACK |
| --- | --- |
| ![DIY Dash dashboard](docs/ui/screenshots/ui-preview-dash.png) | ![DIY Dash track screen](docs/ui/screenshots/ui-preview-track.png) |

| SETTINGS | TILE SETTINGS |
| --- | --- |
| ![Settings home](docs/ui/screenshots/ui-preview-settings-home.png) | ![Full-screen tile editor](docs/ui/screenshots/ui-preview-tile-editor.png) |

| STATUS FLAGS | WARNING MODAL |
| --- | --- |
| ![Configurable status flags](docs/ui/screenshots/ui-preview-flag-tiles.png) | ![Large warning modal](docs/ui/screenshots/ui-preview-warning.png) |

All 13 deterministic 800 × 480 previews are available in
[`docs/ui/screenshots`](docs/ui/screenshots).

## Main features

### Configurable DASH and TRACK pages

- DASH has four small tiles on the left, four on the right, two centered wide
  tiles, and four center-small tiles.
- TRACK has four small tiles on each side and four centered wide tiles.
- Hiding a tile compacts only its logical column or group downward.
- Wide-tile labels and values remain centered.
- Holding a visible tile for approximately 600 ms opens its independent,
  full-screen editor. A short tap does not open it.
- Hidden tiles can be restored from `SETTINGS > LAYOUTS`.
- Every tile stores its parameter, visibility, decimals, warning, temperature
  bar configuration, and flag color independently in NVS.

### Smooth live data

- The visible data page renders at a stable 40 Hz cadence.
- Fast parameters update at 40 Hz, medium parameters at 20 Hz, and slow
  parameters at 10 Hz.
- Numeric presentation interpolates between incoming samples where appropriate.
- Warning and shift-light decisions use raw telemetry at 200 Hz and are not
  delayed by display interpolation.
- DEMO uses a smooth repeatable engine cycle, including a two-second RPM hold
  at 7800 RPM.

### Per-tile warnings

Each numeric tile can independently configure:

- enabled or disabled;
- activation above or below the threshold;
- threshold from `000.0` to `999.0`;
- hysteresis from `000.0` to `999.0`;
- delay from 0 to 10000 ms.

When a limit is exceeded for the configured delay, the UI opens a large red
`WARNING` modal with the parameter name, current value, configured limit, and
the number of additional active warnings. Acknowledging the modal leaves the
related tile highlighted until the signal returns through its hysteresis
boundary. Hidden tiles can still raise warnings.

Changing a numeric tile parameter clears the previous parameter's stale warning.
A new warning configured afterward in the same editor session is preserved.
Warnings are disabled by default in DEMO and must be enabled for the selected
tile when testing this function.

### Temperature bars

Temperature tiles can display a thin continuous colored bar. Each tile has four
native temperature controls:

| Control | Meaning |
| --- | --- |
| `MIN` | Empty end of the fill scale |
| `READY` | Start of the normal operating range |
| `RED` | Independent red over-temperature threshold |
| `MAX` | Full end of the fill scale |

The required order is `MIN < READY < RED <= MAX`. Temperatures use an explicit
signed `+000.0` / `-000.0` format and support `-999.0` to `+999.0`.

- blue: below READY;
- green: normal range;
- yellow: final quarter of the READY-to-RED interval;
- red: at or above RED;
- empty: invalid or unavailable data.

Coolant and oil temperature bars start enabled with default values
`+040.0 / +075.0 / +115.0 / +130.0 °C`. IAT, fuel temperature, and EGT bars
have parameter-specific defaults but start disabled.

### Shift light

DASH and TRACK share the same 12-segment shift-light strip:

- four green segments;
- four yellow segments;
- four red segments;
- adjustable `START`, `RED`, `FLASH`, and `MAX` thresholds;
- 0 to 10000 RPM range in 100 RPM steps;
- required order `START < RED < FLASH <= MAX`;
- optional full-strip red flashing at approximately 4 Hz above `FLASH`.

### Fixed SETTINGS screens

SETTINGS uses separate non-scrolling 800 × 480 screens to keep touch and redraw
performance predictable:

| Category | Functions |
| --- | --- |
| DISPLAY | Software brightness from 20% to 100% |
| DATA & CAN | Explicit DEMO/CAN source, ECU profile, bitrate, and timeout |
| SHIFT LIGHT | Four RPM sliders and flash enable control |
| UNITS | Temperature, pressure, speed, lambda/AFR, and stoichiometric AFR |
| LAYOUTS | Paged DASH/TRACK slot list and hidden-tile restoration |
| SYSTEM | Runtime information, layout reset, and factory reset |

Settings are changed in RAM while the controls are used and written once when
the user leaves the settings screen. Tile settings use `SAVE TILE` and update
the runtime only after persistent storage confirms a successful write.

## Telemetry and selectable data

`VehicleState` is the common model used by DEMO, CAN profiles, warnings, and the
UI. The current registry contains **101 selectable parameters**:

- 35 numeric measurements;
- 66 boolean or status flags.

Numeric data includes RPM, speed, gear, throttle, MAP/boost, lambda, coolant,
oil and intake temperatures, oil/fuel/coolant pressure, battery voltage,
barometric pressure, ethanol content, ignition, injection, airflow, EGT 1–8,
and individual wheel speeds.

DEMO exposes the complete registry. CAN exposes only parameters substantiated
by the selected compiled profile. If a saved tile is not supported by a newly
selected profile, its assignment remains stored and the UI shows `---` or
`UNAVAILABLE` instead of inventing a value.

Boolean tiles show a neutral grey `OFF` state. When active, they use the
per-tile yellow, green, or red color selected by the user. Invalid or stale
flags show `UNAVAILABLE` and never retain the active color.

## CAN profiles

| Profile | Status | Default bitrate | Implemented scope |
| --- | --- | ---: | --- |
| ECUMaster EMU Black | Verified | 1000 kbit/s | Official `0x600–0x607` stream, numeric data and documented states |
| rusEFI verbose | Verified | 500 kbit/s | Official verbose CAN frames, extended engine data and states |
| MaxxECU Default 1.3 | Verified | 500 kbit/s | Official default protocol, extended numeric data and states |
| Haltech Broadcast 2.0 | Verified | 1000 kbit/s | Official big-endian broadcast numeric channels |
| Speeduino Haltech mode | Verified | 500 kbit/s | Populated Haltech-compatible Speeduino channels |
| BMW MS43 Stock | Verified | 500 kbit/s | Stock `0x316`, `0x329`, and `0x545` powertrain frames and verified states |
| Link Generic Dash | Experimental | 1000 kbit/s | Generic Dash frame `0x3E8`; ECU transmission must be configured |
| PSA Citroën C2 VTS engine | Experimental | 500 kbit/s | HS.IS `0x208` and `0x488`; RPM, TPS, CLT, oil and intake temperature |

The BMW profile is stock and passive. It does not require an OLM or K-line
patch and does not use a custom `0x33C` stream. The PSA profile uses engine CAN
only and intentionally excludes VAN, diagnostics, ABS, and uncertain signals.

Exact protocol sources, revisions, frame IDs, mapped channels, and verification
boundaries are recorded in the
[`CAN Profile Source Ledger`](docs/can/profile-sources.md).

## Required hardware

- Waveshare ESP32-S3-Touch-LCD-5 **non-B**;
- USB data cable for programming and serial diagnostics;
- correctly powered display board;
- correctly terminated CAN bus;
- CANH, CANL, and a shared electrical ground with the vehicle/ECU.

CAN/TWAI pin assignment used by the firmware:

| Function | ESP32-S3 GPIO |
| --- | ---: |
| CAN TX | GPIO15 |
| CAN RX | GPIO16 |

> [!CAUTION]
> Confirm voltage levels, grounding, termination, and connector pinout before
> connecting the dashboard to a vehicle. The firmware uses receive-only CAN,
> but incorrect electrical wiring can still damage the board or other modules.

## Install the ready-to-use firmware

### Method 1 — GitHub Release and complete BIN (recommended)

This method does not require downloading or compiling the source code.

1. Download
   [`DIY-Dash-ESP32-S3-Touch-LCD-5-full.bin`](https://github.com/barcu00/DIY-dash-5/releases/download/v0.2.1/DIY-Dash-ESP32-S3-Touch-LCD-5-full.bin).
2. Install Python 3 and Espressif's flashing tool:

   ```bash
   python -m pip install --upgrade esptool
   ```

3. Connect the ESP32-S3 board by USB and identify its serial port.
4. Erase the target flash:

   ```bash
   python -m esptool --chip esp32s3 --port COM5 erase_flash
   ```

5. Flash the complete image at `0x0`:

   ```bash
   python -m esptool --chip esp32s3 --port COM5 --baud 921600 write_flash \
     0x0 DIY-Dash-ESP32-S3-Touch-LCD-5-full.bin
   ```

Replace `COM5` with the board port. On Linux it is commonly similar to
`/dev/ttyACM0` or `/dev/ttyUSB0`; on macOS it is commonly under `/dev/cu.*`.
If the board does not enter download mode automatically, use its BOOT/RESET
procedure and retry.

### Method 2 — GitHub Actions artifact

Every push and pull request runs the complete firmware workflow:

1. Open [GitHub Actions](https://github.com/barcu00/DIY-dash-5/actions/workflows/build-firmware.yml).
2. Select a successful run for the required commit.
3. Download the `DIY-Dash-firmware` artifact.
4. Extract it and flash the included full BIN at `0x0` using Method 1.

Actions artifacts are temporary development builds. For ordinary installation,
use the permanent full BIN attached to the v0.2.1 release. Release pages contain
only the complete ready-to-flash image; component binaries remain development
outputs inside Actions artifacts and local PlatformIO builds.

## Download, compile, and upload the complete project

The repository is a complete PlatformIO project. It can be cloned with Git or
downloaded as a ZIP, opened directly in PlatformIO, compiled without manually
installing Arduino libraries, and uploaded to the ESP32-S3 over USB. Required
libraries and their pinned versions are declared in [`platformio.ini`](platformio.ini).

### PlatformIO IDE in Visual Studio Code

1. Install [Visual Studio Code](https://code.visualstudio.com/).
2. Install the **PlatformIO IDE** extension.
3. Download the repository:
   - use `git clone https://github.com/barcu00/DIY-dash-5.git`, or
   - choose **Code > Download ZIP** on GitHub and extract the complete archive.
4. In VS Code select **File > Open Folder** and open the directory containing
   `platformio.ini`.
5. Wait for PlatformIO to install the pinned ESP32 platform and libraries.
6. Select the `waveshare_5` environment.
7. Use **PlatformIO: Build** to compile the complete software.
8. Connect the board and use **PlatformIO: Upload** to flash it.

Generated files are placed in `.pio/build/waveshare_5/`, including:

- `firmware.bin`;
- `bootloader.bin`;
- `partitions.bin`;
- `DIY-Dash-ESP32-S3-Touch-LCD-5-full.bin`;
- `flash-layout.json`.

### PlatformIO command line

```bash
git clone https://github.com/barcu00/DIY-dash-5.git
cd DIY-dash-5
python -m pip install platformio==6.1.18
pio run -e waveshare_5
pio run -e waveshare_5 -t upload --upload-port COM5
```

To run the complete native regression suite before the ESP32 build:

```bash
pio test -e native
```

Replace `COM5` with the detected ESP32-S3 port. The build post-script creates
the merged 16 MB image and flash layout from PlatformIO's actual build metadata.

## First start and configuration

1. After flashing, reset or power-cycle the board.
2. Confirm that DASH appears at 800 × 480 and touch navigation works.
3. Open `SETTINGS > DATA & CAN`.
4. Select `DEMO` for UI testing, or select `CAN` and the matching ECU profile.
5. Set the required CAN bitrate and timeout.
6. Leave the settings page to save the configuration.
7. Hold a tile to configure its parameter, warning, visibility, decimal count,
   temperature bar, or flag color.
8. Test warnings in DEMO by enabling a warning and choosing a threshold crossed
   by the generated signal range.

CAN mode never silently falls back to DEMO. A disconnected, invalid, or stale
CAN signal is displayed as unavailable.

## Software architecture

```text
ESP32 TWAI / CAN frames
        │
        ▼
    CanDriver
        │
        ▼
Table-driven ECU decoder ──────── DEMO telemetry
        │                              │
        └──────────► VehicleState ◄────┘
                         │
             ┌───────────┼───────────┐
             ▼           ▼           ▼
       Tile warnings  Shift light  LVGL tile views
             │           │           │
             └───────────┴───────────┘
                         │
                         ▼
                  800 × 480 display
```

Hardware-specific display, touch, and CAN access are isolated from the native
C++ telemetry, configuration, decoder, warning, and presentation models. CAN
definitions remain table-driven; raw byte layouts do not leak into UI code.

## Build and verification status

The current v0.2.1 firmware was built from commit
[`5d561ed`](https://github.com/barcu00/DIY-dash-5/commit/5d561edc4229067916ff45e37cbaf7651880f462).

| Check | Result |
| --- | ---: |
| Native test cases | 176 / 176 passed |
| Push workflow | [Passed](https://github.com/barcu00/DIY-dash-5/actions/runs/34399159652) |
| Pull-request workflow | [Passed](https://github.com/barcu00/DIY-dash-5/actions/runs/34399164683) |
| Static RAM | 131,656 / 327,680 bytes — 40.2% |
| Application flash | 711,144 / 6,553,600 bytes — 10.9% |
| Complete flash image | 16,777,216 bytes |

GitHub Actions performs:

1. 176 native C++ tests;
2. Python packaging and UI contract tests;
3. Waveshare ESP32-S3 firmware compilation;
4. deterministic rendering of all UI previews;
5. merged BIN and flash-layout generation;
6. firmware artifact upload.

## Software stack

- C++17;
- PlatformIO 6.1.18;
- pioarduino ESP32 platform 53.3.11 / Arduino-ESP32 3.1.1;
- LVGL 8.4.0;
- ESP32_Display_Panel 1.0.4;
- ESP32_IO_Expander 1.1.1;
- esp-lib-utils 0.2.0.

Versions are pinned in [`platformio.ini`](platformio.ini) to keep CI and
developer builds reproducible.

## Repository map

| Path | Purpose |
| --- | --- |
| `src/app` | Runtime orchestration |
| `src/board` | RGB display, GT911 touch, brightness, and diagnostics |
| `src/can` | ESP32 TWAI receive-only driver |
| `src/ecu` | CAN profiles and table-driven decoding |
| `src/telemetry` | Parameter registry, DEMO data, and `VehicleState` |
| `src/alarms` | Per-tile warning state machine |
| `src/settings` | Defaults, validation, schema migration, and NVS storage |
| `src/ui` | Layout, editor, settings, tiles, shift light, and presentation |
| `test` | Native regression and protocol fixture tests |
| `scripts` | Firmware packaging, UI rendering, and contract tests |
| `docs/can` | Profile source ledger and mapping boundaries |
| `docs/ui` | User guide and current screen previews |

## Current limitations

- This is not yet a production-certified automotive instrument.
- The non-B Waveshare board is the only current hardware target.
- Link Generic Dash and PSA C2 mappings remain experimental until verified
  against captured traffic from the exact target vehicle/ECU.
- The firmware does not implement BMW MS43 OLM, a custom `0x33C` stream, or
  K-line acquisition.
- The PSA profile does not decode VAN traffic.
- microSD logging and profile import/export are not implemented in v0.2.1.
- CI cannot validate electrical wiring, display timing under every condition,
  vehicle-bus compatibility, or long-duration thermal stability.

## Physical-board acceptance checklist

After flashing, verify:

1. the board boots without a reset loop;
2. DASH renders at the full 800 × 480 resolution;
3. touch switches between DASH, TRACK, and every SETTINGS category;
4. long-press opens a tile editor while a short tap does nothing;
5. DEMO data, temperature bars, warnings, and the shift light behave correctly;
6. repeated slider and brightness interaction causes no artifacts;
7. saved settings survive a restart;
8. CAN changes from waiting/offline only after receiving a mapped frame;
9. disconnecting CAN invalidates stale values and never activates DEMO;
10. the display remains responsive during a minimum 15-minute test.

## Documentation

- [Dashboard configuration guide](docs/ui/dashboard-config-guide.md)
- [CAN profile source ledger](docs/can/profile-sources.md)
- [Current v0.2.1 release](https://github.com/barcu00/DIY-dash-5/releases/tag/v0.2.1)
- [GitHub Actions firmware workflow](.github/workflows/build-firmware.yml)
