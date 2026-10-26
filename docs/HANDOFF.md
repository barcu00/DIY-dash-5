# DIY Dash — handoff notes

Working branch: `feature/tach-bar-layouts` (repo `barcu00/diy-dash-5`).
The owner writes in Polish; answer in Polish, directly and precisely.

## Ground rules

- **Build and test only on GitHub Actions**, not locally:
  - `.github/workflows/build-firmware.yml` — native unit tests + firmware for
    both boards. Memory usage (`RAM:` / `Flash:`) is published as notice
    annotations; failing native tests are annotated too
    (`gh api repos/barcu00/diy-dash-5/check-runs/<id>/annotations`).
  - `.github/workflows/publish-binaries.yml` — on push to
    `feature/tach-bar-layouts`, commits images to branch `firmware-builds`
    (`5-inch/`, `7-inch/`, `BUILD.txt`). Fetch with
    `git fetch origin firmware-builds && git archive origin/firmware-builds`.
- **Every change ends with a binary package**: `*-full.bin` (flash at 0x0)
  and `*-app-update.bin` (keeps settings), for 5" and 7".
- 7" keeps `ARDUINO_USB_CDC_ON_BOOT=0` — **do not re-enable it**. The 5"
  target must stay independently buildable and unchanged in behaviour.
- Preserve all existing layouts; do not simplify configurability.
- UI text is English only.

## Internal RAM on the 7" board is tight

Commit `cf9f23e` added ~5.7 KB of static internal RAM (a 90-sample trend
buffer in every tile) and the 7" board stopped booting (backlight, black
screen, occasional bright bands). Fixed in `59cab33` / `57619df`:

- Static RAM after the fix: ~94.4 KB (last good before: ~98.7 KB,
  broken: ~104.4 KB). Watch the `RAM:` annotation on every build.
- Large buffers go to PSRAM (`heap_caps_malloc(MALLOC_CAP_SPIRAM)`) and are
  allocated only when needed.
- Avoid `AppConfig` (~12.6 KB) temporaries on the stack; use the heap or
  `AppConfig::setDefaults(config)` in place.
- Boot log prints `Internal RAM free=… largest=… DMA free=…` before the
  panel starts (UART, 115200).

## Current state

- 11 dashboard layouts incl. OEM SPORT, TRACK FOCUS, ENDURANCE,
  ANALOG CLASSIC, ANALOG TWIN; tach-bar shift light.
- Round dials: `src/ui/round_geometry.*`; 7" pixel aspect correction
  `HardwareProfile::round_scale_x ≈ 0.9286` (5" = 1.0).
- Analog/chamfered tiles (`TileView::arrangeStyled`) use a fixed value box
  sized for the widest text of the same shape; re-layout only on shape/unit
  change.
- ENDURANCE rows: trend arrow + sparkline (`src/ui/trend_model.*`,
  90 samples). Window 5/10/15/20 s (default 15 s) in
  SETTINGS > DISPLAY > ENDURANCE TREND. Buffer allocated in PSRAM on first
  sample, released when the page leaves the screen, on layout change or hide.
- Config schema 11 (`trend_window_s`), migrations by stored size in
  `src/settings/config_repository.cpp` (v10 → v11 copies everything).
- Fonts: `dash_italic_*` (Barlow Condensed italic), `dash_label_*` (Barlow),
  generated with lv_font_conv 1.5.3.

## Open pull requests

- #2 `feature/review-fixes-odometer` → `feature/racechrono-ble-monitor`
- #3 `feature/tach-bar-layouts` → `feature/review-fixes-odometer`
