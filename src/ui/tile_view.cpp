#include "tile_view.h"
#include <cstdio>
#include "telemetry/parameter_registry.h"
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
}
void TileView::hide() { if (root_) lv_obj_add_flag(root_, LV_OBJ_FLAG_HIDDEN); }
void TileView::update(const TileConfig& config, const UnitSettings& units,
                      const VehicleState& state, bool warning_active) {
    const SignalValue& signal = state.get(config.parameter);
    if (!signal.valid) {
        lv_label_set_text(value_, "---");
        lv_label_set_text(unit_, "");
    } else {
        const PresentedValue p = UnitPresenter::present(config.parameter, signal.value, units);
        char format[8]; char buffer[32];
        std::snprintf(format, sizeof(format), "%%.%uf", static_cast<unsigned>(config.decimals));
        std::snprintf(buffer, sizeof(buffer), format, static_cast<double>(p.value));
        lv_label_set_text(value_, buffer); lv_label_set_text(unit_, p.unit);
    }
    lv_obj_set_style_border_width(root_, warning_active ? 3 : 1, 0);
    lv_obj_set_style_border_color(root_, warning_active ? UiTheme::red() : UiTheme::border(), 0);
}
TileAddress TileView::address() const { return address_; }
