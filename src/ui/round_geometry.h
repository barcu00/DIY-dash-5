#pragma once

#include <lvgl.h>

// Drawing helpers for round elements (dials, gauges) that must look round on
// panels with non-square pixels: x distances from the centre are scaled by
// the board's round_scale_x, so circles become ellipses in pixel space.
namespace RoundGeometry {

float scaleX();
// Overrides the board value (preview tools only).
void setScaleX(float scale);

lv_point_t point(lv_point_t center, float radius, float degrees);
// Thick arc band between radius +/- width/2, angles in LVGL convention
// (0 = 3 o'clock, clockwise); end may exceed 360.
void arc(lv_draw_ctx_t* ctx, lv_point_t center, float radius, float width,
         float start, float end, lv_color_t color, lv_opa_t opa = LV_OPA_COVER);
void disc(lv_draw_ctx_t* ctx, lv_point_t center, float radius, lv_color_t color,
          lv_opa_t opa = LV_OPA_COVER);
// Covering box of a ring sector (for partial invalidation).
lv_area_t sectorBounds(lv_point_t center, float inner, float outer, float start, float end);

}  // namespace RoundGeometry
