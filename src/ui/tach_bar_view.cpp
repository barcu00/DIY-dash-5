#include "tach_bar_view.h"

#include <algorithm>
#include <cstdio>

#include "ui/dashboard_layout.h"
#include "ui/ui_theme.h"

namespace {
constexpr int kWidth = 784;
constexpr int kBarHeight = 24;
constexpr int kHeight = 48;

lv_color_t tachColor(TachColor color) {
    switch (color) {
        case TachColor::Yellow: return UiTheme::yellow();
        case TachColor::Red: return UiTheme::red();
        case TachColor::Off: return lv_color_hex(0x161C22);
        case TachColor::Normal: break;
    }
    return lv_color_hex(0xE4E8EC);
}

int xFor(uint32_t rpm, uint16_t maximum) {
    return static_cast<int>(std::min<uint32_t>(rpm, maximum) * (kWidth - 1) / maximum);
}
}  // namespace

void TachBarView::create(lv_obj_t* parent) {
    root_ = lv_obj_create(parent);
    lv_obj_set_pos(root_, 8, 8);
    lv_obj_set_size(root_, kWidth, kHeight);
    lv_obj_set_style_bg_opa(root_, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(root_, 0, 0);
    lv_obj_set_style_pad_all(root_, 0, 0);
    lv_obj_clear_flag(root_, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(root_, drawEvent, LV_EVENT_DRAW_MAIN, this);
    lv_obj_add_flag(root_, LV_OBJ_FLAG_HIDDEN);
}

void TachBarView::apply(DashboardLayout layout, uint16_t scale_max) {
    scale_max_ = normalizedRpmScale(scale_max);
    state_ = TachBarState{};
    initialized_ = false;
    filter_.reset();
    if (layoutHasTachBar(layout)) lv_obj_clear_flag(root_, LV_OBJ_FLAG_HIDDEN);
    else lv_obj_add_flag(root_, LV_OBJ_FLAG_HIDDEN);
    lv_obj_invalidate(root_);
}

void TachBarView::invalidateFill(uint16_t previous, uint16_t current) {
    lv_area_t area;
    lv_obj_get_coords(root_, &area);
    const int lo = std::min(previous, current) * (kWidth - 1) / 1000;
    const int hi = std::max(previous, current) * (kWidth - 1) / 1000;
    lv_area_t dirty{static_cast<lv_coord_t>(area.x1 + std::max(0, lo - 2)), area.y1,
                    static_cast<lv_coord_t>(area.x1 + std::min(kWidth - 1, hi + 2)),
                    static_cast<lv_coord_t>(area.y1 + kBarHeight - 1)};
    lv_obj_invalidate_area(root_, &dirty);
}

void TachBarView::update(const SignalValue& raw, uint32_t now_ms,
                         const ShiftLightConfig& shift) {
    if (root_ == nullptr || lv_obj_has_flag(root_, LV_OBJ_FLAG_HIDDEN)) return;
    if (shift.start_rpm != shift_.start_rpm || shift.red_rpm != shift_.red_rpm ||
        shift.flash_rpm != shift_.flash_rpm || shift.flash_enabled != shift_.flash_enabled) {
        shift_ = shift;
        lv_obj_invalidate(root_);  // threshold markers moved
    }
    const SignalValue rpm = filter_.sample(raw, now_ms, 50U);
    const TachBarState next =
        tachBarState(rpm.value, rpm.valid, scale_max_, shift_, now_ms);
    if (initialized_ && next == state_) return;
    const bool recolour = !initialized_ || next.color != state_.color ||
                          next.flashing != state_.flashing;
    const uint16_t previous = state_.fill_permille;
    state_ = next;
    initialized_ = true;
    if (recolour) invalidateFill(0U, 1000U);
    else invalidateFill(previous, next.fill_permille);
}

void TachBarView::drawEvent(lv_event_t* event) {
    auto* self = static_cast<TachBarView*>(lv_event_get_user_data(event));
    if (self == nullptr || self->root_ == nullptr) return;
    lv_draw_ctx_t* ctx = lv_event_get_draw_ctx(event);
    lv_area_t area;
    lv_obj_get_coords(self->root_, &area);

    lv_draw_rect_dsc_t track;
    lv_draw_rect_dsc_init(&track);
    track.radius = 4;
    track.bg_color = lv_color_hex(0x12171C);
    const lv_area_t bar{area.x1, area.y1, area.x2,
                        static_cast<lv_coord_t>(area.y1 + kBarHeight - 1)};
    lv_draw_rect(ctx, &track, &bar);

    const int fill = self->state_.fill_permille * (kWidth - 1) / 1000;
    if (fill > 0) {
        lv_draw_rect_dsc_t lit;
        lv_draw_rect_dsc_init(&lit);
        lit.radius = 4;
        lit.bg_color = tachColor(self->state_.color);
        const lv_area_t filled{area.x1, area.y1,
                               static_cast<lv_coord_t>(area.x1 + fill),
                               static_cast<lv_coord_t>(area.y1 + kBarHeight - 1)};
        lv_draw_rect(ctx, &lit, &filled);
    }

    // Scale: a tick every 1000 rpm with its number, threshold markers.
    lv_draw_line_dsc_t tick;
    lv_draw_line_dsc_init(&tick);
    tick.width = 1;
    tick.color = UiTheme::muted();
    lv_draw_label_dsc_t text;
    lv_draw_label_dsc_init(&text);
    text.font = &lv_font_montserrat_12;
    text.color = UiTheme::muted();
    text.align = LV_TEXT_ALIGN_CENTER;
    const uint16_t maximum = self->scale_max_;
    for (uint32_t rpm = 0U; rpm <= maximum; rpm += 1000U) {
        const int x = area.x1 + xFor(rpm, maximum);
        const lv_point_t a{static_cast<lv_coord_t>(x), static_cast<lv_coord_t>(area.y1 + kBarHeight + 1)};
        const lv_point_t b{static_cast<lv_coord_t>(x), static_cast<lv_coord_t>(area.y1 + kBarHeight + 6)};
        lv_draw_line(ctx, &tick, &a, &b);
        char label[4];
        std::snprintf(label, sizeof(label), "%u", static_cast<unsigned>(rpm / 1000U));
        const int left = std::clamp(x - 12, static_cast<int>(area.x1), static_cast<int>(area.x2) - 24);
        const lv_area_t box{static_cast<lv_coord_t>(left), static_cast<lv_coord_t>(area.y1 + kBarHeight + 8),
                            static_cast<lv_coord_t>(left + 24), static_cast<lv_coord_t>(area.y2)};
        lv_draw_label(ctx, &text, &box, label, nullptr);
    }
    const struct { uint16_t rpm; lv_color_t color; } marks[] = {
        {self->shift_.start_rpm, UiTheme::yellow()},
        {self->shift_.red_rpm, UiTheme::red()}};
    for (const auto& mark : marks) {
        if (mark.rpm == 0U || mark.rpm > maximum) continue;
        lv_draw_rect_dsc_t m;
        lv_draw_rect_dsc_init(&m);
        m.bg_color = mark.color;
        const int x = area.x1 + xFor(mark.rpm, maximum);
        const lv_area_t box{static_cast<lv_coord_t>(x - 1), static_cast<lv_coord_t>(area.y1 + kBarHeight + 1),
                            static_cast<lv_coord_t>(x + 1), static_cast<lv_coord_t>(area.y1 + kBarHeight + 10)};
        lv_draw_rect(ctx, &m, &box);
    }
}
