#include "tile_view.h"
#include <cstdio>
#include <cstring>
#include "telemetry/parameter_registry.h"
#include "ui/tile_refresh_policy.h"
#include "ui/ui_theme.h"
#include "ui/unit_presenter.h"
#include "ui/tile_view_policy.h"
void TileView::create(lv_obj_t* parent, TileAddress address, lv_event_cb_t callback) {
    address_ = address;
    root_ = lv_obj_create(parent);
    lv_obj_set_style_bg_color(root_, UiTheme::panel(), 0);
    lv_obj_set_style_border_color(root_, UiTheme::border(), 0);
    lv_obj_set_style_border_width(root_, 1, 0);
    lv_obj_set_style_radius(root_, 8, 0);
    lv_obj_set_style_pad_all(root_, 8, 0);
    lv_obj_clear_flag(root_, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(root_, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(root_, callback, LV_EVENT_LONG_PRESSED, this);
    lv_obj_t* stripe = lv_obj_create(root_);
    lv_obj_set_pos(stripe, 0, 0);
    lv_obj_set_size(stripe, 4, 88);
    lv_obj_set_style_bg_color(stripe, UiTheme::blue(), 0);
    lv_obj_set_style_border_width(stripe, 0, 0);
    lv_obj_clear_flag(stripe, LV_OBJ_FLAG_SCROLLABLE);
    title_ = lv_label_create(root_);
    lv_obj_set_style_text_font(title_, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(title_, UiTheme::muted(), 0);
    value_ = lv_label_create(root_);
    lv_obj_set_style_text_font(value_, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(
        value_, lv_color_hex(tileViewPolicy(TileSize::Small).value_rgb), 0);
    unit_ = lv_label_create(root_);
    lv_obj_set_style_text_font(unit_, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(unit_, UiTheme::muted(), 0);
}
void TileView::apply(const TileConfig& config, const TileGeometry& geometry) {
    const bool centered = geometry.size == TileSize::Wide;
    lv_obj_clear_flag(root_, LV_OBJ_FLAG_HIDDEN);
    lv_obj_set_pos(root_, geometry.x, geometry.y);
    lv_obj_set_size(root_, geometry.width, geometry.height);
    lv_label_set_text(title_, parameterDescriptor(config.parameter).short_name);
    lv_obj_set_width(title_, geometry.width - 24);
    lv_obj_set_style_text_align(title_, centered ? LV_TEXT_ALIGN_CENTER : LV_TEXT_ALIGN_LEFT, 0);
    lv_obj_align(title_, centered ? LV_ALIGN_TOP_MID : LV_ALIGN_TOP_LEFT,
                 centered ? 0 : 10, 2);
    const TileViewPolicy policy = tileViewPolicy(geometry.size);
    lv_obj_set_width(value_, geometry.width - (centered ? 70 : 28));
    lv_obj_set_style_text_align(
        value_, policy.value_centered ? LV_TEXT_ALIGN_CENTER : LV_TEXT_ALIGN_LEFT, 0);
    lv_obj_align(value_, policy.value_centered ? LV_ALIGN_CENTER : LV_ALIGN_LEFT_MID,
                 policy.value_centered ? 0 : 10, 10);
    lv_obj_align(unit_, centered ? LV_ALIGN_RIGHT_MID : LV_ALIGN_BOTTOM_RIGHT,
                 centered ? -18 : -4, centered ? 9 : -2);
    display_filter_.reset();
    refresh_initialized_ = false;
    value_text_initialized_ = false;
    unit_text_initialized_ = false;
    raw_valid_initialized_ = false;
    warning_initialized_ = false;
}
void TileView::hide() { if (root_) lv_obj_add_flag(root_, LV_OBJ_FLAG_HIDDEN); }
void TileView::update(const TileConfig& config, const UnitSettings& units,
                      const VehicleState& state, bool warning_active,
                      uint32_t now_ms) {
    if (!warning_initialized_ || warning_active != last_warning_active_) {
        lv_obj_set_style_border_width(root_, warning_active ? 3 : 1, 0);
        lv_obj_set_style_border_color(
            root_, warning_active ? UiTheme::red() : UiTheme::border(), 0);
        warning_initialized_ = true;
        last_warning_active_ = warning_active;
    }

    const SignalValue& raw = state.get(config.parameter);
    const bool validity_changed = !raw_valid_initialized_ ||
                                  raw.valid != last_raw_valid_;
    const uint32_t interval_ms = tileRefreshIntervalMs(config.parameter);
    if (!refresh_initialized_) {
        next_refresh_ms_ = now_ms;
        refresh_initialized_ = true;
    }
    if (!validity_changed &&
        static_cast<int32_t>(now_ms - next_refresh_ms_) < 0) {
        return;
    }
    if (static_cast<int32_t>(now_ms - next_refresh_ms_) >= 0) {
        const uint32_t elapsed = now_ms - next_refresh_ms_;
        next_refresh_ms_ += (elapsed / interval_ms + 1U) * interval_ms;
    }
    raw_valid_initialized_ = true;
    last_raw_valid_ = raw.valid;

    const SignalValue signal = display_filter_.sample(
        raw, now_ms, interval_ms * 2U);
    char value_text[32] = "---";
    const char* unit_text = "";
    if (signal.valid) {
        const PresentedValue p = UnitPresenter::present(
            config.parameter, signal.value, units);
        char format[8]; char buffer[32];
        std::snprintf(format, sizeof(format), "%%.%uf", static_cast<unsigned>(config.decimals));
        std::snprintf(buffer, sizeof(buffer), format, static_cast<double>(p.value));
        std::snprintf(value_text, sizeof(value_text), "%s", buffer);
        unit_text = p.unit;
    }

    if (!value_text_initialized_ ||
        std::strcmp(last_value_text_.data(), value_text) != 0) {
        lv_label_set_text(value_, value_text);
        std::snprintf(last_value_text_.data(), last_value_text_.size(),
                      "%s", value_text);
        value_text_initialized_ = true;
    }
    if (!unit_text_initialized_ ||
        std::strcmp(last_unit_text_.data(), unit_text) != 0) {
        lv_label_set_text(unit_, unit_text);
        std::snprintf(last_unit_text_.data(), last_unit_text_.size(),
                      "%s", unit_text);
        unit_text_initialized_ = true;
    }
}
TileAddress TileView::address() const { return address_; }
