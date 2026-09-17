# Five selectable dashboard layouts

Approved by the user on 2026-09-17, including permission to implement.

- Preserve Classic Dash and Classic Track. Add Analog Style (reference 9), Side Gear (5), and Strip Style (4).
- Each of DASH and TRACK can select any of these five presets, including the same preset on both pages.
- Keep independently persisted tile banks for every preset on each page. Long press opens the existing full-screen editor. Numeric warnings, flag colors, units, temperature bars, and hiding remain available.
- Compact hidden tiles downward inside their column/group; do not move tiles across groups.
- One shared RPM scale starts at zero and ends at a configurable positive maximum of at most 10,000 RPM. Default 10,000; 100-RPM steps. Keep this setting independent of shift-light thresholds.
- Analog dial and RPM strip have a fixed RPM source. Numeric tiles remain configurable.
- Strip Style has no logo or marketing header. Navigation uses gauge, checkered-flag, and gear icons alongside DASH, TRACK, SETTINGS.
- Save settings on leaving their screen; no flash writes while adjusting a slider.
- Preserve existing NVS schemas 1–5 through migration, including all old 14 DASH and 12 TRACK tiles.
- Reuse LVGL objects and invalidate only changed values/indicator areas; pause dashboard rendering in settings and editor.
- Run unit tests, packaging tests, firmware build, and deterministic 800×480 previews on GitHub Actions, not locally. Do not create a public release or merge main without a separate request.

Implementation uses a shared layout model for preset names, slot counts, and geometry; a dedicated low-object-count RPM view; and 14-slot alternate banks. The original default banks retain their frozen 14/12-slot sizes for migration compatibility. Accessors expose only each preset's active slots. Extra layout banks are data only, not persistent LVGL screens. Default page selections remain unchanged.
