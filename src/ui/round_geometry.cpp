#include "round_geometry.h"

#include <algorithm>
#include <cmath>

#include "board/hardware_profile.h"

namespace RoundGeometry {
namespace {
constexpr float kPi = 3.14159265358979323846f;
float g_scale = currentHardwareProfile().round_scale_x;

lv_point_t at(lv_point_t c, float r, float degrees) {
    const float a = degrees * kPi / 180.0f;
    return {static_cast<lv_coord_t>(std::lround(c.x + r * std::cos(a) * g_scale)),
            static_cast<lv_coord_t>(std::lround(c.y + r * std::sin(a)))};
}
}  // namespace

float scaleX() { return g_scale; }
void setScaleX(float scale) { g_scale = scale; }

lv_point_t point(lv_point_t center, float radius, float degrees) {
    return at(center, radius, degrees);
}

void arc(lv_draw_ctx_t* ctx, lv_point_t center, float radius, float width,
         float start, float end, lv_color_t color, lv_opa_t opa) {
    if (end <= start || width <= 0.0f) return;
    if (g_scale == 1.0f) {
        // Square pixels: the native (faster, anti-aliased) arc is exact.
        lv_draw_arc_dsc_t d;
        lv_draw_arc_dsc_init(&d);
        d.color = color;
        d.opa = opa;
        d.width = static_cast<lv_coord_t>(std::lround(width));
        const int s = static_cast<int>(std::lround(start)) % 360;
        const int e = static_cast<int>(std::lround(end)) % 360;
        if (s == e && end - start >= 359.5f) {
            lv_draw_arc(ctx, &d, &center, static_cast<uint16_t>(std::lround(radius + width / 2)), 0, 360);
        } else {
            lv_draw_arc(ctx, &d, &center, static_cast<uint16_t>(std::lround(radius + width / 2)),
                        static_cast<uint16_t>(s), static_cast<uint16_t>(e));
        }
        return;
    }
    // Elliptical band: draw as near-convex quads of at most 4 degrees.
    lv_draw_rect_dsc_t d;
    lv_draw_rect_dsc_init(&d);
    d.bg_color = color;
    d.bg_opa = opa;
    const float inner = radius - width / 2.0f, outer = radius + width / 2.0f;
    const int steps = std::max(1, static_cast<int>(std::ceil((end - start) / 4.0f)));
    for (int i = 0; i < steps; ++i) {
        const float a0 = start + (end - start) * i / steps;
        const float a1 = start + (end - start) * (i + 1) / steps;
        const lv_point_t quad[4] = {at(center, outer, a0), at(center, outer, a1),
                                    at(center, inner, a1), at(center, inner, a0)};
        lv_draw_polygon(ctx, &d, quad, 4);
    }
}

void disc(lv_draw_ctx_t* ctx, lv_point_t center, float radius, lv_color_t color, lv_opa_t opa) {
    lv_draw_rect_dsc_t d;
    lv_draw_rect_dsc_init(&d);
    d.bg_color = color;
    d.bg_opa = opa;
    if (g_scale == 1.0f) {
        d.radius = LV_RADIUS_CIRCLE;
        const lv_area_t a{static_cast<lv_coord_t>(center.x - radius), static_cast<lv_coord_t>(center.y - radius),
                          static_cast<lv_coord_t>(center.x + radius), static_cast<lv_coord_t>(center.y + radius)};
        lv_draw_rect(ctx, &d, &a);
        return;
    }
    // Ellipse as 1 px scan lines, limited to the area being redrawn
    // (LVGL polygons are capped at a handful of mask edges).
    const lv_area_t clip = *ctx->clip_area;
    const int top = std::max<int>(clip.y1, static_cast<int>(std::floor(center.y - radius)));
    const int bottom = std::min<int>(clip.y2, static_cast<int>(std::ceil(center.y + radius)));
    for (int y = top; y <= bottom; ++y) {
        const float dy = static_cast<float>(y - center.y);
        const float span = radius * radius - dy * dy;
        if (span <= 0.0f) continue;
        const int half = static_cast<int>(std::lround(std::sqrt(span) * g_scale));
        const lv_area_t row{static_cast<lv_coord_t>(center.x - half), static_cast<lv_coord_t>(y),
                            static_cast<lv_coord_t>(center.x + half), static_cast<lv_coord_t>(y)};
        lv_draw_rect(ctx, &d, &row);
    }
}

lv_area_t sectorBounds(lv_point_t center, float inner, float outer, float start, float end) {
    int x1 = LV_COORD_MAX, y1 = LV_COORD_MAX, x2 = LV_COORD_MIN, y2 = LV_COORD_MIN;
    for (float a = start;; a = std::min(end, a + 3.0f)) {
        for (float r : {inner, outer}) {
            const lv_point_t p = at(center, r, a);
            x1 = std::min<int>(x1, p.x); x2 = std::max<int>(x2, p.x);
            y1 = std::min<int>(y1, p.y); y2 = std::max<int>(y2, p.y);
        }
        if (a >= end) break;
    }
    return {static_cast<lv_coord_t>(x1), static_cast<lv_coord_t>(y1),
            static_cast<lv_coord_t>(x2), static_cast<lv_coord_t>(y2)};
}

}  // namespace RoundGeometry
