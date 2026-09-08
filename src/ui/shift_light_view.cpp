#include "shift_light_view.h"
#include "ui/shift_light_model.h"
#include "ui/ui_theme.h"

namespace {
bool sameStates(const ShiftSegmentStates& left,
                const ShiftSegmentStates& right) {
    for (std::size_t i = 0U; i < left.size(); ++i) {
        if (left[i].lit != right[i].lit ||
            left[i].color != right[i].color) {
            return false;
        }
    }
    return true;
}
}

void ShiftLightView::create(lv_obj_t* parent) {
    state_initialized_ = false;
    strip_ = lv_obj_create(parent);
    lv_obj_set_pos(strip_, 8, 8);
    lv_obj_set_size(strip_, 786, 16);
    lv_obj_set_style_bg_opa(strip_, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(strip_, 0, 0);
    lv_obj_set_style_pad_all(strip_, 0, 0);
    lv_obj_clear_flag(strip_, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(strip_, drawEvent, LV_EVENT_DRAW_MAIN, this);
}

void ShiftLightView::drawEvent(lv_event_t* event) {
    auto* self = static_cast<ShiftLightView*>(lv_event_get_user_data(event));
    if (self == nullptr || self->strip_ == nullptr) return;

    lv_draw_ctx_t* draw_ctx = lv_event_get_draw_ctx(event);
    lv_area_t strip_area;
    lv_obj_get_coords(self->strip_, &strip_area);
    for (std::size_t i = 0U; i < self->last_states_.size(); ++i) {
        lv_draw_rect_dsc_t dsc;
        lv_draw_rect_dsc_init(&dsc);
        dsc.radius = 4;
        dsc.bg_opa = LV_OPA_COVER;
        dsc.bg_color = lv_color_hex(0x202830);
        if (self->last_states_[i].lit) {
            if (self->last_states_[i].color == ShiftColor::Green)
                dsc.bg_color = UiTheme::green();
            if (self->last_states_[i].color == ShiftColor::Yellow)
                dsc.bg_color = UiTheme::yellow();
            if (self->last_states_[i].color == ShiftColor::Red)
                dsc.bg_color = UiTheme::red();
        }
        lv_area_t segment{
            static_cast<lv_coord_t>(strip_area.x1 + static_cast<int>(i) * 66),
            strip_area.y1,
            static_cast<lv_coord_t>(strip_area.x1 + static_cast<int>(i) * 66 + 59),
            strip_area.y2};
        lv_draw_rect(draw_ctx, &dsc, &segment);
    }
}
void ShiftLightView::update(uint16_t rpm, bool rpm_valid, uint32_t now_ms,
                            const ShiftLightConfig& config) {
    const ShiftSegmentStates states =
        ShiftLightModel::segments(rpm, rpm_valid, now_ms, config);
    if (state_initialized_ && sameStates(states, last_states_)) {
        return;
    }
    last_states_ = states;
    state_initialized_ = true;
    if (strip_ != nullptr) lv_obj_invalidate(strip_);
}
