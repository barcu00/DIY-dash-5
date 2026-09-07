#include "shift_light_view.h"
#include "ui/shift_light_model.h"
#include "ui/ui_theme.h"
void ShiftLightView::create(lv_obj_t* parent) {
    for (std::size_t i = 0U; i < segments_.size(); ++i) {
        segments_[i] = lv_obj_create(parent);
        lv_obj_set_pos(segments_[i], 8 + static_cast<int>(i) * 66, 8);
        lv_obj_set_size(segments_[i], 60, 16);
        lv_obj_set_style_border_width(segments_[i], 0, 0);
        lv_obj_set_style_radius(segments_[i], 4, 0);
        lv_obj_set_style_bg_color(segments_[i], lv_color_hex(0x202830), 0);
        lv_obj_clear_flag(segments_[i], LV_OBJ_FLAG_SCROLLABLE);
    }
}
void ShiftLightView::update(uint16_t rpm, const ShiftLightConfig& config) {
    const ShiftSegmentStates states = ShiftLightModel::segments(rpm, config);
    for (std::size_t i = 0U; i < segments_.size(); ++i) {
        lv_color_t color = lv_color_hex(0x202830);
        if (states[i].lit) {
            if (states[i].color == ShiftColor::Green) color = UiTheme::green();
            if (states[i].color == ShiftColor::Yellow) color = UiTheme::yellow();
            if (states[i].color == ShiftColor::Red) color = UiTheme::red();
        }
        lv_obj_set_style_bg_color(segments_[i], color, 0);
    }
}
