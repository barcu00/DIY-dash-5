# Approved six-layout dashboard implementation plan

> Execute inline in this session. Use executing-plans and a read-only code review before delivery.

**Goal:** Implement the accepted D Analog, Modern Motorsport E, circular Strip and centered cyan-rail tiles without losing saved settings.

**Architecture:** Append E to the existing layout enum and banks; keep all previous enum values stable. Migrate persisted v6/v7 layouts by copying the original four alternatives per page into v8 defaults. Keep static tachometer scales separate from dynamic indicators and only invalidate changed areas.

**Tech Stack:** C++17, LVGL 8.4, ESP32-S3, PlatformIO, GitHub Actions.

**Spec:** Accepted LVGL images from commits ed2c172 and 991e653, plus the user's approval to implement E as the sixth selectable layout.

## Global constraints

- Tests, firmware compilation and LVGL rendering run on GitHub only.
- Continue in the existing dashboard-dev checkout; no release or main merge.
- E is independently selectable for DASH and TRACK, with six editable right rows.
- Preserve hiding/packing, long-press editing, warnings, flags, CAN profiles, units and exit-save.
- Analog D: radial white rectangular index, RPM at center, no needle or inner decorative circle.
- E: continuous upper 180-degree arc, slim active band, dim future zones, white progress cap and internal RPM.
- Strip: equal circular sectors, constant thickness and angular gaps, partial progress changes brightness, not shape.
- Whole-thousand scale labels only; 7500 RPM maximum does not produce a 7.5 label.
- Existing shift flash remains 4 Hz. Shared scale remains independent of shift limits.
- Cyan tile rails in every preset; numeric values, labels and units optically centered in compact rows.

## Task 1: Sixth layout storage and backward compatibility

Files: src/settings/app_config.h, src/settings/config_repository.cpp, src/ui/dashboard_layout.{h,cpp}, src/ui/tile_engine.cpp, src/ui/ui.cpp, src/ui/shift_light_view.cpp, test/test_dashboard_layout/test_main.cpp, test/test_config_repository/test_main.cpp.

- [x] Add native behavior tests for enum value 5, six right-row placements, independent page banks, E save/load and v7 migration preserving sound and all old banks.
- [x] Push test-only commit and inspect failing GitHub native logs before implementing.
- [x] Append ModernMotorsport, expand alternatives to five banks, bump schema to 8; copy_n old bank arrays in v6/v7 migration. Add dropdown choice, placement reuse and integrated shift behavior.
- [x] Run full GitHub native tests and packaging tests.

## Task 2: Real LVGL styles and redraw safety

Files: src/ui/rpm_scale_view.{h,cpp}, src/ui/tile_view.{h,cpp}, src/ui/dashboard_layout.{h,cpp}, design/production-preview/{render.cpp,convert.py}.

- [x] Add actual framebuffer assertions for centered white digits and cyan rails, shared scale and new E scenes.
- [x] Render on GitHub against old views to observe failures.
- [x] Use native circular arcs for Analog and E. Precompute Strip congruent circular sector polygons with clipped geometry, rather than per-frame supersampled prototype rasterization.
- [x] Draw radial D rectangle, E progress cap and continuous fill; invalidate old and new marker regions, E changed arc strip and changed Strip sectors. Repaint the entire indicator on flash/validity changes.
- [x] Center visible glyph bounds and value/unit group on compact rows, refitting and repositioning only on text changes. Preserve flag and unavailable captions.
- [x] Add E idle, redline, scale 7500, warning/flag/unavailable scenarios and incremental-versus-full framebuffer comparisons.
- [x] Run production rendering and full firmware build on GitHub; visually inspect downloaded images.

## Task 3: Review and firmware handoff

- [x] Request a read-only reviewer for changes since 991e653; fix important findings.
- [x] Record successful native/Python counts, production render assertions and build memory usage.
- [x] Download the full firmware binary, verify length and SHA256; show real firmware screen images and the local full-bin link with offset 0x0 and NVS overwrite caution.
