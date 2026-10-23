#include "status_band_view.h"

#include <cstdio>
#include <cstring>

#include "ui/ui_theme.h"

namespace {
lv_color_t toneColor(StatusTone tone) {
    switch (tone) {
        case StatusTone::Alert: return UiTheme::red();
        case StatusTone::Caution: return UiTheme::yellow();
        case StatusTone::Info: return UiTheme::blue();
        case StatusTone::Normal: break;
    }
    return UiTheme::green();
}
}  // namespace

void StatusBandView::create(lv_obj_t* parent) {
    root_ = lv_obj_create(parent);
    lv_obj_set_style_bg_color(root_, UiTheme::panel(), 0);
    lv_obj_set_style_border_color(root_, UiTheme::border(), 0);
    lv_obj_set_style_border_width(root_, 1, 0);
    lv_obj_set_style_radius(root_, 8, 0);
    lv_obj_set_style_pad_all(root_, 0, 0);
    lv_obj_clear_flag(root_, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
    dot_ = lv_obj_create(root_);
    lv_obj_set_size(dot_, 12, 12);
    lv_obj_set_style_radius(dot_, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_border_width(dot_, 0, 0);
    lv_obj_clear_flag(dot_, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
    title_ = lv_label_create(root_);
    lv_obj_set_style_text_font(title_, &lv_font_montserrat_20, 0);
    lv_label_set_long_mode(title_, LV_LABEL_LONG_DOT);
    detail_ = lv_label_create(root_);
    lv_obj_set_style_text_font(detail_, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(detail_, UiTheme::muted(), 0);
    lv_label_set_long_mode(detail_, LV_LABEL_LONG_DOT);
    lv_obj_add_flag(root_, LV_OBJ_FLAG_HIDDEN);
}

void StatusBandView::apply(DashboardLayout layout) {
    initialized_ = false;
    last_title_.fill('\0');
    last_detail_.fill('\0');
    int x = 0, y = 0, w = 0, h = 0;
    if (layout == DashboardLayout::OemSport) {
        x = 8; y = 332; w = 784; h = 90;  // right part holds three inline tiles
    } else if (layout == DashboardLayout::Endurance) {
        x = 8; y = 350; w = 784; h = 72;
    } else {
        lv_obj_add_flag(root_, LV_OBJ_FLAG_HIDDEN);
        return;
    }
    lv_obj_clear_flag(root_, LV_OBJ_FLAG_HIDDEN);
    lv_obj_set_pos(root_, x, y);
    lv_obj_set_size(root_, w, h);
    const int text_width = layout == DashboardLayout::OemSport ? 548 : 740;
    lv_obj_align(dot_, LV_ALIGN_TOP_LEFT, 16, h / 2 - 18);
    lv_obj_set_width(title_, text_width);
    lv_obj_align(title_, LV_ALIGN_TOP_LEFT, 38, h / 2 - 26);
    lv_obj_set_width(detail_, text_width);
    lv_obj_align(detail_, LV_ALIGN_TOP_LEFT, 38, h / 2 + 2);
}

bool StatusBandView::visible() const {
    return root_ != nullptr && !lv_obj_has_flag(root_, LV_OBJ_FLAG_HIDDEN);
}

void StatusBandView::show(StatusTone tone, const char* title, const char* detail) {
    if (!visible()) return;
    if (!initialized_ || tone != tone_) {
        lv_obj_set_style_bg_color(dot_, toneColor(tone), 0);
        lv_obj_set_style_text_color(title_, tone == StatusTone::Normal
            ? UiTheme::text() : toneColor(tone), 0);
        lv_obj_set_style_border_color(root_, tone == StatusTone::Alert
            ? UiTheme::red() : tone == StatusTone::Caution ? UiTheme::yellow()
            : UiTheme::border(), 0);
        tone_ = tone;
    }
    if (!initialized_ || std::strcmp(last_title_.data(), title) != 0) {
        lv_label_set_text(title_, title);
        std::snprintf(last_title_.data(), last_title_.size(), "%s", title);
    }
    if (!initialized_ || std::strcmp(last_detail_.data(), detail) != 0) {
        lv_label_set_text(detail_, detail);
        std::snprintf(last_detail_.data(), last_detail_.size(), "%s", detail);
    }
    initialized_ = true;
}
