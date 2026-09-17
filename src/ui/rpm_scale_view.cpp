#include "rpm_scale_view.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include "ui/dashboard_layout.h"
#include "ui/ui_theme.h"

namespace {
constexpr float kPi = 3.14159265358979323846f;
void line(lv_draw_ctx_t* ctx, lv_point_t from, lv_point_t to,
          lv_color_t color, int width = 2) {
    lv_draw_line_dsc_t dsc;
    lv_draw_line_dsc_init(&dsc);
    dsc.color = color;
    dsc.width = width;
    dsc.round_start = dsc.round_end = 1;
    lv_draw_line(ctx, &dsc, &from, &to);
}
void label(lv_draw_ctx_t* ctx, int x, int y, const char* text,
           const lv_font_t* font = &lv_font_montserrat_12) {
    lv_draw_label_dsc_t dsc;
    lv_draw_label_dsc_init(&dsc);
    dsc.color = UiTheme::text();
    dsc.font = font;
    dsc.align = LV_TEXT_ALIGN_CENTER;
    lv_area_t area{static_cast<lv_coord_t>(x - 28), static_cast<lv_coord_t>(y - 8),
                   static_cast<lv_coord_t>(x + 28), static_cast<lv_coord_t>(y + 16)};
    lv_draw_label(ctx, &dsc, &area, text, nullptr);
}
lv_point_t polar(lv_point_t center, float radius, float degrees) {
    const float radians = degrees * kPi / 180.0f;
    return {static_cast<lv_coord_t>(center.x + radius * std::cos(radians)),
            static_cast<lv_coord_t>(center.y + radius * std::sin(radians))};
}
int curveY(float x) {
    const float normalized = 2.0f * x - 1.0f;
    return static_cast<int>(12 + 46 * normalized * normalized);
}
void transparent(lv_obj_t* obj) {
    lv_obj_set_style_bg_opa(obj, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(obj, 0, 0);
    lv_obj_set_style_pad_all(obj, 0, 0);
    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
}
}  // namespace

void RpmScaleView::create(lv_obj_t* parent) {
    root_ = lv_obj_create(parent);
    transparent(root_);
    lv_obj_add_event_cb(root_, drawScale, LV_EVENT_DRAW_MAIN, this);
    indicator_ = lv_obj_create(root_);
    transparent(indicator_);
    lv_obj_add_event_cb(indicator_, drawIndicator, LV_EVENT_DRAW_MAIN, this);
    value_ = lv_label_create(root_);
    lv_obj_set_style_text_font(value_, &lv_font_montserrat_48, 0);
    lv_obj_set_style_text_color(value_, UiTheme::text(), 0);
    lv_obj_set_style_text_align(value_, LV_TEXT_ALIGN_CENTER, 0);
    caption_ = lv_label_create(root_);
    lv_label_set_text(caption_, "RPM");
    lv_obj_set_style_text_font(caption_, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(caption_, UiTheme::text(), 0);
    lv_obj_add_flag(root_, LV_OBJ_FLAG_HIDDEN);
}

void RpmScaleView::apply(DashboardLayout layout, uint16_t maximum) {
    layout_ = layout;
    maximum_ = normalizedRpmScale(maximum);
    fill_ = 0;
    valid_ = flashing_ = red_phase_ = initialized_ = false;
    last_text_.fill('\0');
    filter_.reset();
    if (layout != DashboardLayout::AnalogStyle && layout != DashboardLayout::SideGear &&
        layout != DashboardLayout::StripStyle) {
        lv_obj_add_flag(root_, LV_OBJ_FLAG_HIDDEN);
        return;
    }
    lv_obj_clear_flag(root_, LV_OBJ_FLAG_HIDDEN);
    const bool analog = layout == DashboardLayout::AnalogStyle;
    lv_obj_set_pos(root_, layout == DashboardLayout::SideGear ? 142 : 8, analog ? 36 : 8);
    lv_obj_set_size(root_, analog ? 416 : layout == DashboardLayout::SideGear ? 650 : 784,
                    analog ? 386 : layout == DashboardLayout::SideGear ? 96 : 140);
    lv_obj_set_pos(indicator_, 0, 0);
    lv_obj_set_size(indicator_, lv_obj_get_width(root_), analog ? 386 :
        layout == DashboardLayout::SideGear ? 60 : 96);
    if (analog) {
        lv_obj_clear_flag(value_, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(caption_, LV_OBJ_FLAG_HIDDEN);
        lv_obj_set_width(value_, 240);
        lv_label_set_text(value_, "---");
        lv_obj_align(value_, LV_ALIGN_BOTTOM_MID, 0, -44);
        lv_obj_align(caption_, LV_ALIGN_BOTTOM_MID, 0, -16);
    } else {
        lv_obj_add_flag(value_, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(caption_, LV_OBJ_FLAG_HIDDEN);
    }
    lv_obj_invalidate(root_);
}

lv_point_t RpmScaleView::needleEnd(uint16_t fill) const {
    lv_area_t a;
    lv_obj_get_coords(indicator_, &a);
    return polar({static_cast<lv_coord_t>(a.x1 + 208),
                  static_cast<lv_coord_t>(a.y1 + 190)}, 158.0f,
                  135.0f + 270.0f * fill / 1000.0f);
}

void RpmScaleView::invalidateNeedle(uint16_t fill) {
    lv_area_t a;
    lv_obj_get_coords(indicator_, &a);
    const auto end = needleEnd(fill);
    const lv_coord_t cx = a.x1 + 208, cy = a.y1 + 190;
    lv_area_t dirty{static_cast<lv_coord_t>(std::min(cx, end.x) - 8),
        static_cast<lv_coord_t>(std::min(cy, end.y) - 8),
        static_cast<lv_coord_t>(std::max(cx, end.x) + 8),
        static_cast<lv_coord_t>(std::max(cy, end.y) + 8)};
    lv_obj_invalidate_area(indicator_, &dirty);
}

void RpmScaleView::update(const SignalValue& raw, uint32_t now_ms,
                          const ShiftLightConfig& shift) {
    if (lv_obj_has_flag(root_, LV_OBJ_FLAG_HIDDEN)) return;
    const bool flashing = raw.valid && shift.flash_enabled && raw.value >= shift.flash_rpm;
    const bool red = flashing && now_ms % 250U < 125U;
    const bool animation_due = !initialized_ || now_ms - last_update_ms_ >= 25U || raw.valid != valid_;
    if (!animation_due && flashing == flashing_ && red == red_phase_) return;
    const SignalValue rpm = filter_.sample(raw, now_ms, 50U);
    const uint16_t fill = rpmScaleFill(rpm.value, rpm.valid, maximum_);
    const bool analog = layout_ == DashboardLayout::AnalogStyle;
    if (!initialized_ || valid_ != rpm.valid || fill != fill_ ||
        (!analog && (flashing != flashing_ || red != red_phase_))) {
        if (analog) {
            invalidateNeedle(fill_);
            invalidateNeedle(fill);
        } else {
            lv_area_t a;
            lv_obj_get_coords(indicator_, &a);
            if (initialized_ && valid_ == rpm.valid && flashing == flashing_ && red == red_phase_) {
                const int width = lv_obj_get_width(indicator_) - 8;
                a.x2 = a.x1 + 4 + (width * std::max(fill_, fill)) / 1000 + 4;
                a.x1 += std::max(0, 4 + (width * std::min(fill_, fill)) / 1000 - 4);
            }
            lv_obj_invalidate_area(indicator_, &a);
        }
        fill_ = fill;
        valid_ = rpm.valid;
        flashing_ = flashing;
        red_phase_ = red;
    }
    if (analog && animation_due) {
        char text[16] = "---";
        if (rpm.valid) std::snprintf(text, sizeof(text), "%.0f", static_cast<double>(rpm.value));
        if (std::strcmp(text, last_text_.data()) != 0) {
            lv_label_set_text(value_, text);
            std::snprintf(last_text_.data(), last_text_.size(), "%s", text);
        }
    }
    if (animation_due) last_update_ms_ = now_ms;
    initialized_ = true;
}

void RpmScaleView::drawScale(lv_event_t* event) {
    auto* self = static_cast<RpmScaleView*>(lv_event_get_user_data(event));
    auto* ctx = lv_event_get_draw_ctx(event);
    lv_area_t a;
    lv_obj_get_coords(self->root_, &a);
    const bool analog = self->layout_ == DashboardLayout::AnalogStyle;
    const int width = lv_obj_get_width(self->root_) - 8;
    const lv_point_t center{static_cast<lv_coord_t>(a.x1 + 208), static_cast<lv_coord_t>(a.y1 + 190)};
    for (int i = 0; i <= (analog ? 50 : 10); ++i) {
        const float fraction = static_cast<float>(i) / (analog ? 50 : 10);
        if (analog) {
            const float angle = 135.0f + 270.0f * fraction;
            line(ctx, polar(center, 178, angle), polar(center, i % 5 == 0 ? 164 : 171, angle),
                fraction >= 0.8f ? UiTheme::red() : UiTheme::text(), i % 5 == 0 ? 3 : 1);
            if (i % 5 != 0) continue;
        } else {
            const int x = a.x1 + 4 + static_cast<int>(width * fraction);
            const int y = a.y1 + (self->layout_ == DashboardLayout::StripStyle ? curveY(fraction) + 38 : 62);
            line(ctx, {static_cast<lv_coord_t>(x), static_cast<lv_coord_t>(y)},
                 {static_cast<lv_coord_t>(x), static_cast<lv_coord_t>(y + 6)}, UiTheme::muted(), 1);
        }
        char text[16];
        std::snprintf(text, sizeof(text), "%.1f", self->maximum_ * fraction / 1000.0);
        if (std::strstr(text, ".0")) *std::strstr(text, ".0") = '\0';
        if (analog) {
            auto p = polar(center, 142, 135.0f + 270.0f * fraction);
            label(ctx, p.x, p.y, text, &lv_font_montserrat_16);
        } else {
            const int x = a.x1 + 4 + static_cast<int>(width * fraction);
            const int y = a.y1 + (self->layout_ == DashboardLayout::StripStyle ? curveY(fraction) + 55 : 79);
            label(ctx, std::clamp(x, static_cast<int>(a.x1 + 28), static_cast<int>(a.x2 - 28)), y, text);
        }
    }
    if (!analog) label(ctx, a.x2 - 72, a.y1 + 8, "RPM x1000");
}

void RpmScaleView::drawIndicator(lv_event_t* event) {
    auto* self = static_cast<RpmScaleView*>(lv_event_get_user_data(event));
    auto* ctx = lv_event_get_draw_ctx(event);
    lv_area_t a;
    lv_obj_get_coords(self->indicator_, &a);
    if (self->layout_ == DashboardLayout::AnalogStyle) {
        if (!self->valid_) return;
        lv_point_t center{static_cast<lv_coord_t>(a.x1 + 208), static_cast<lv_coord_t>(a.y1 + 190)};
        line(ctx, center, self->needleEnd(self->fill_), UiTheme::red(), 6);
        lv_draw_rect_dsc_t dsc;
        lv_draw_rect_dsc_init(&dsc);
        dsc.radius = LV_RADIUS_CIRCLE;
        dsc.bg_color = UiTheme::border();
        lv_area_t pivot{static_cast<lv_coord_t>(center.x - 8), static_cast<lv_coord_t>(center.y - 8),
                         static_cast<lv_coord_t>(center.x + 8), static_cast<lv_coord_t>(center.y + 8)};
        lv_draw_rect(ctx, &dsc, &pivot);
        return;
    }
    const int width = lv_obj_get_width(self->indicator_) - 8;
    constexpr int count = 36;
    for (int i = 0; i < count; ++i) {
        const float from = static_cast<float>(i) / count;
        const float to = static_cast<float>(i + 1) / count;
        const int x1 = a.x1 + 4 + static_cast<int>(width * from);
        const int x2 = a.x1 + 4 + static_cast<int>(width * to) - 2;
        const bool curved = self->layout_ == DashboardLayout::StripStyle;
        const int y1 = a.y1 + (curved ? curveY(from) : 24);
        const int y2 = a.y1 + (curved ? curveY(to) : 24);
        lv_draw_rect_dsc_t dsc;
        lv_draw_rect_dsc_init(&dsc);
        dsc.bg_color = lv_color_hex(0x202830);
        const lv_point_t points[] = {
            {static_cast<lv_coord_t>(x1), static_cast<lv_coord_t>(y1)},
            {static_cast<lv_coord_t>(x2), static_cast<lv_coord_t>(y2)},
            {static_cast<lv_coord_t>(x2), static_cast<lv_coord_t>(y2 + 26)},
            {static_cast<lv_coord_t>(x1), static_cast<lv_coord_t>(y1 + 26)}};
        lv_draw_polygon(ctx, &dsc, points, 4);
        const float progress = self->fill_ / 1000.0f;
        const float lit = self->flashing_ ? (self->red_phase_ ? 1.0f : 0.0f) :
            self->valid_ ? std::clamp((progress - from) / (to - from), 0.0f, 1.0f) : 0.0f;
        if (lit <= 0.0f) continue;
        dsc.bg_color = self->flashing_ ? UiTheme::red() :
            i < 12 ? UiTheme::green() : i < 24 ? UiTheme::yellow() : UiTheme::red();
        lv_point_t lit_points[4];
        std::copy(std::begin(points), std::end(points), lit_points);
        lit_points[1].x = lit_points[2].x = static_cast<lv_coord_t>(x1 + (x2 - x1) * lit);
        lit_points[1].y = static_cast<lv_coord_t>(y1 + (y2 - y1) * lit);
        lit_points[2].y = lit_points[1].y + 26;
        lv_draw_polygon(ctx, &dsc, lit_points, 4);
    }
}
