# Approved LVGL Style Integration Plan

> Execute inline in the existing development checkout. User approved all five LVGL prototypes; no further design gate is needed.

**Goal:** Make the firmware match the approved LVGL prototypes while preserving all configuration and live-data behavior.

**Architecture:** Keep persisted banks/schema 6 unchanged. Update placement geometry, tile presentation, shared RPM drawing, navigation and fonts. Render actual production views in a standalone LVGL framebuffer harness on GitHub for visual QA rather than using Pillow reconstruction.

**Tech stack:** C++17, LVGL 8.4, ESP32-S3, Unity, GitHub Actions.

**Spec:** User-approved `design/lvgl-mockups/mockup.c`, approval commits `2382bc0` and `c0c3444`.

## Constraints

- English UI, five presets independently assigned to DASH/TRACK, old settings preserved.
- Long-press editor, warnings, flags, temperature bars, hidden-group packing and exit-save remain intact.
- Shared RPM scale 100–10000, independent shift thresholds; flashing remains 4 Hz.
- Tests and firmware compilation only on GitHub. No main merge or release.
- No per-sample allocation or rebuilding; numerical and indicator layers invalidate only changed regions.

## Task 1: Approved placements

- [x] Add failing `test_approved_style_placements` in `test/test_dashboard_layout/test_main.cpp`: Analog first row (464,16,328,62), Side Gear (8,16,136,292), Strip left (8,178,158,76), center (176,178,218,244). Update hidden-column expected Y values to 262/346; no center movement.
- [x] Push and observe assertion failures on GitHub.
- [x] Update `src/ui/tile_engine.cpp` to those literal geometries, side main (170,145,292,155)/(480,145,292,155), bottom (8+158*i,316,150,106), analog rows 16+68*i. Preserve classic layouts and banks.
- [x] Re-run native and Python tests on GitHub.

## Task 2: Production views

- [x] Copy generated licensed Rajdhani digit fonts 48/64/96/140 to `src/ui/fonts`; add declarations with Montserrat fallback. No schema changes.
- [x] Update `src/ui/ui_theme.h` to black panels, white digits, blue-grey thin frames, cyan/green/yellow/orange/red accents; set tile padding to zero and explicit coordinates matching the approved renderer.
- [x] Update `src/ui/tile_view.cpp` for centered small/wide digits, compact rows and outline-free heroes; preserve flags, unavailable captions, warnings, temperature bars and callbacks. Hero captions use RPM/speed units; other parameters keep their names.
- [x] Update `src/ui/rpm_scale_view.cpp` for the analog colored ring, triangular needle, side shared frame, slanted curved strip and fine ticks. Keep interpolated fill, raw flash decision, and bounded invalidation. Analog flash uses needle/value accent, avoiding full-ring repaint.
- [x] Update navigation in `src/ui/ui.cpp` to 20px labels, thin top/separator lines, proper padding and cyan underline. Existing icons remain vector-drawn.
- [x] Build firmware on GitHub and inspect logs.

## Task 3: Actual production-view previews and handoff

- [x] Add standalone native LVGL harness using real `TileView`, `RpmScaleView`, `ShiftLightView`, `TileEngine` and model sources. Compile/render on GitHub; produce five 800×480 PNGs via lossless PPM conversion only.
- [x] Verify geometry, font fit, compact flag OFF/ON/unavailable and temperature/warning styling in captured framebuffers; physical smoothness remains a board test.
- [x] Run independent read-only code review, fix important findings, and re-run GitHub build.
- [x] Download full BIN and previews, update development docs/screens, record tested commit and provide handoff. Leave main/release untouched.

## Verification record

- Tested code: `f8091cd4d133f70820b5f74c0003fe8d4cb55877`.
- [GitHub firmware run](https://github.com/barcu00/DIY-dash-5/actions/runs/35215149831): success; 185 native tests, 24 Python tests, Waveshare ESP32-S3 build and full binary packaging.
- [Actual LVGL render run](https://github.com/barcu00/DIY-dash-5/actions/runs/35215149834): success; 14 scenes, flash colors, compact flag separation, 10000-RPM width and unavailable-caption bounds.
- Independent read-only review found no blocking regression after fixes.
- Full image: 16777216 bytes, offset `0x0`, SHA-256 `E059A895999DE1A6442754077A915E03721F0EA1C6514EAC556A0E5D7AAB66B1`.
- Linker-reported static internal RAM: 142464/327680 bytes (43.5%); flash application: 814216/6553600 bytes. These are not runtime heap/PSRAM measurements.
- Five framebuffer captures copied into `docs/ui/screenshots/firmware-*.png`. Downloaded full image is in workspace artifacts `firmware-approved-f8091cd`.
- No local test/build execution. Physical smoothness, touch and CAN still require board testing. No main merge or release performed.
