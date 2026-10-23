#include "tile_view.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include "telemetry/parameter_registry.h"
#include "ui/flag_tile_model.h"
#include "ui/tile_refresh_policy.h"
#include "ui/ui_theme.h"
#include "ui/unit_presenter.h"
#include "ui/tile_view_policy.h"
#include "ui/fonts/numeric_fonts.h"
#include "ui/tile_value_format.h"
namespace {
lv_color_t temperatureBarColor(TemperatureBarZone zone) {
    switch (zone) {
        case TemperatureBarZone::Cold: return UiTheme::blue();
        case TemperatureBarZone::Normal: return UiTheme::green();
        case TemperatureBarZone::Warm: return UiTheme::yellow();
        case TemperatureBarZone::Hot: return UiTheme::red();
        case TemperatureBarZone::Unavailable: return UiTheme::muted();
    }
    return UiTheme::muted();
}
bool statusText(const char* text) {
    return std::strcmp(text, "UNAVAILABLE") == 0 ||
           std::strcmp(text, "DATA LOST") == 0;
}
int opticalTextY(const char* text,const lv_font_t* font,int center) {
    int top=font->line_height,bottom=0;uint32_t index=0;
    while(text[index]) {
        const uint32_t cp=_lv_txt_encoded_next(text,&index);
        lv_font_glyph_dsc_t glyph;
        if(lv_font_get_glyph_dsc(font,&glyph,cp,0) && glyph.box_h) {
            const int y=font->line_height-font->base_line-glyph.box_h-glyph.ofs_y;
            top=std::min(top,y);bottom=std::max(bottom,y+glyph.box_h);
        }
    }
    return center-(bottom>top ? (top+bottom)/2:font->line_height/2);
}
}  // namespace

void TileView::create(lv_obj_t* parent, TileAddress address, lv_event_cb_t callback) {
    address_ = address;
    root_ = lv_obj_create(parent);
    lv_obj_set_style_bg_color(root_, UiTheme::panel(), 0);
    lv_obj_set_style_border_color(root_, UiTheme::border(), 0);
    lv_obj_set_style_border_width(root_, 1, 0);
    lv_obj_set_style_radius(root_, 8, 0);
    lv_obj_set_style_pad_all(root_, 0, 0);
    lv_obj_clear_flag(root_, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(root_, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(root_, callback, LV_EVENT_LONG_PRESSED, this);
    stripe_ = lv_obj_create(root_);
    lv_obj_set_pos(stripe_, 0, 0);
    lv_obj_set_size(stripe_, 4, 88);
    lv_obj_set_style_bg_color(stripe_, UiTheme::border(), 0);
    lv_obj_set_style_border_width(stripe_, 0, 0);
    lv_obj_clear_flag(stripe_, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
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
    temperature_bar_ = lv_bar_create(root_);
    lv_obj_clear_flag(temperature_bar_, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    lv_bar_set_range(temperature_bar_, 0, 1000);
    lv_obj_set_style_bg_color(
        temperature_bar_, UiTheme::border(), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(temperature_bar_, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_bg_color(
        temperature_bar_, UiTheme::green(), LV_PART_INDICATOR);
    lv_obj_set_style_bg_opa(
        temperature_bar_, LV_OPA_COVER, LV_PART_INDICATOR);
    lv_obj_set_style_border_width(temperature_bar_, 0, 0);
    lv_obj_set_style_radius(temperature_bar_, 4, LV_PART_MAIN);
    lv_obj_set_style_radius(temperature_bar_, 4, LV_PART_INDICATOR);
    lv_obj_add_flag(temperature_bar_, LV_OBJ_FLAG_HIDDEN);
    gauge_ = lv_obj_create(root_);
    lv_obj_set_style_bg_opa(gauge_, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(gauge_, 0, 0);
    lv_obj_set_style_pad_all(gauge_, 0, 0);
    lv_obj_clear_flag(gauge_, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(gauge_, drawGauge, LV_EVENT_DRAW_MAIN, this);
    lv_obj_add_flag(gauge_, LV_OBJ_FLAG_HIDDEN);
}
void TileView::apply(const TileConfig& config, const TileGeometry& geometry) {
    const bool hero = geometry.size == TileSize::Hero || geometry.size == TileSize::GearHero;
    const bool row = geometry.size == TileSize::CompactRow;
    const bool centered = geometry.size == TileSize::Wide || hero ||
                          geometry.size == TileSize::CenteredSmall;
    const bool is_flag = parameterDescriptor(config.parameter).kind ==
                         ParameterKind::Flag;
    const bool inline_tile = geometry.size == TileSize::Inline;
    const bool gauge = geometry.size == TileSize::Gauge;
    const bool large = geometry.size == TileSize::Large;
    size_ = geometry.size;
    geometry_ = geometry;
    base_border_ = geometry.size != TileSize::Hero && !inline_tile;
    lv_obj_clear_flag(root_, LV_OBJ_FLAG_HIDDEN);
    lv_obj_set_pos(root_, geometry.x, geometry.y);
    lv_obj_set_size(root_, geometry.width, geometry.height);
    lv_obj_set_style_bg_opa(root_, base_border_ ? LV_OPA_COVER : LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(root_, base_border_ ? 1 : 0, 0);
    lv_obj_set_style_radius(root_, 6, 0);
    lv_obj_set_style_bg_color(root_, UiTheme::panel(), 0);
    lv_obj_set_pos(stripe_, row ? 6 : 4, row ? 7 : 5);
    lv_obj_set_size(stripe_, 4, geometry.height - (row ? 14 : 10));
    lv_obj_set_style_radius(stripe_, 2, 0);
    lv_obj_set_style_bg_color(stripe_, UiTheme::border(), 0);
    if (hero || inline_tile) lv_obj_add_flag(stripe_, LV_OBJ_FLAG_HIDDEN);
    else lv_obj_clear_flag(stripe_, LV_OBJ_FLAG_HIDDEN);
    const lv_font_t* value_font = &race_digits_48;
    if (geometry.size == TileSize::Wide) value_font = &race_digits_64;
    else if (geometry.size == TileSize::Hero)
        value_font = &race_digits_96;
    else if (geometry.size == TileSize::GearHero)
        value_font = config.parameter == ParameterId::Gear
            ? (geometry.height >= 200 ? &race_digits_140 : &race_digits_96)
            : &race_digits_64;
    else if (large) value_font = geometry.height >= 150 ? &race_digits_96 : &race_digits_64;
    else if (gauge) value_font = &race_digits_64;
    else if (inline_tile) value_font = geometry.height >= 50 ? &race_digits_48 : &lv_font_montserrat_24;
    if (is_flag) value_font = row ? &lv_font_montserrat_20 : geometry.height < 90 ? &lv_font_montserrat_16 : &lv_font_montserrat_24;
    default_value_font_ = value_font;
    lv_obj_set_style_text_font(value_, value_font, 0);
    lv_label_set_long_mode(value_, LV_LABEL_LONG_CLIP);
    lv_obj_set_style_text_font(title_, geometry.size == TileSize::GearHero ? &lv_font_montserrat_24 : &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(title_, hero || row ? UiTheme::text() : UiTheme::muted(), 0);
    lv_label_set_text(title_, parameterDescriptor(config.parameter).short_name);
    lv_label_set_long_mode(title_, LV_LABEL_LONG_CLIP);
    lv_obj_clear_flag(title_, LV_OBJ_FLAG_HIDDEN);
    lv_obj_set_width(title_, geometry.width - 24);
    lv_obj_set_style_text_align(
        title_, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(title_, LV_ALIGN_TOP_MID, 0, 9);
    const TileViewPolicy policy = tileViewPolicy(geometry.size);
    lv_obj_set_width(value_, is_flag ? 92
                                    : geometry.width - (centered ? 70 : 28));
    lv_obj_set_style_bg_opa(value_, LV_OPA_TRANSP, 0);
    lv_obj_set_style_pad_all(value_, 0, 0);
    lv_obj_set_style_radius(value_, 0, 0);
    lv_obj_set_style_text_color(
        value_, UiTheme::text(), 0);
    lv_obj_set_style_text_align(
        value_, (policy.value_centered || is_flag) ? LV_TEXT_ALIGN_CENTER
                                                  : LV_TEXT_ALIGN_LEFT, 0);
    lv_obj_align(value_, (policy.value_centered || is_flag) ? LV_ALIGN_CENTER
                                                           : LV_ALIGN_LEFT_MID,
                 (policy.value_centered || is_flag) ? 0 : 10, 10);
    const bool temperature_bar_visible = config.temperature_bar.enabled &&
        parameterDescriptor(config.parameter).native_unit == NativeUnit::Celsius;
    lv_obj_set_width(unit_, is_flag ? geometry.width - 24 : LV_SIZE_CONTENT);
    lv_obj_set_style_text_align(unit_, is_flag ? LV_TEXT_ALIGN_CENTER
                                              : LV_TEXT_ALIGN_LEFT, 0);
    lv_obj_align(unit_, is_flag ? LV_ALIGN_BOTTOM_MID :
                 (centered ? LV_ALIGN_RIGHT_MID : LV_ALIGN_BOTTOM_RIGHT),
                 is_flag ? 0 : (centered ? -18 : -4),
                 is_flag ? -2 :
                 (centered ? 9 : (temperature_bar_visible ? -14 : -2)));
    if (temperature_bar_visible) {
        lv_obj_clear_flag(temperature_bar_, LV_OBJ_FLAG_HIDDEN);
        lv_obj_set_size(temperature_bar_, geometry.width - 28, 3);
        lv_obj_set_style_radius(temperature_bar_, 1, LV_PART_MAIN);
        lv_obj_set_style_radius(temperature_bar_, 1, LV_PART_INDICATOR);
        lv_obj_align(temperature_bar_, LV_ALIGN_BOTTOM_MID, 0, -4);
    } else {
        lv_obj_add_flag(temperature_bar_, LV_OBJ_FLAG_HIDDEN);
    }
    if (hero && !is_flag) {
        lv_obj_set_width(value_,geometry.size == TileSize::Hero ? geometry.width : geometry.width - 24);
        lv_obj_set_style_text_align(value_, LV_TEXT_ALIGN_CENTER, 0);
        const bool gear = geometry.size == TileSize::GearHero;
        const bool compact = geometry.height < 200;
        lv_obj_align(value_, LV_ALIGN_TOP_MID, 0,
                     gear ? (compact ? 28 : 100) : geometry.height > 200 ? 60 : 10);
        lv_obj_align(title_, LV_ALIGN_TOP_MID, 0, gear ? (compact ? 6 : 26) : 2);
        if (!gear && (config.parameter == ParameterId::Rpm || config.parameter == ParameterId::Speed ||
                      geometry.height < 110))
            lv_obj_add_flag(title_, LV_OBJ_FLAG_HIDDEN);
        lv_obj_set_style_text_font(unit_, &lv_font_montserrat_24, 0);
        lv_obj_align(unit_, LV_ALIGN_BOTTOM_MID, 0, gear ? -18 : geometry.height > 200 ? -43 : -12);
    } else {
        lv_obj_set_style_text_font(unit_, &lv_font_montserrat_16, 0);
        if (is_flag) {
            lv_obj_set_style_text_font(unit_,&lv_font_montserrat_12,0);
            if (!row && geometry.height < 90) {
                lv_obj_align(title_,LV_ALIGN_TOP_MID,0,7);
                lv_obj_align(value_,LV_ALIGN_CENTER,0,4);
            }
        }
        if (row && !is_flag) {
            lv_obj_set_width(title_, 106);
            lv_obj_set_style_text_align(title_, LV_TEXT_ALIGN_LEFT, 0);
            lv_obj_align(title_, LV_ALIGN_TOP_LEFT, 22, 22);
            lv_obj_set_width(value_, 123);
            lv_obj_set_style_text_align(value_, LV_TEXT_ALIGN_RIGHT, 0);
            lv_obj_align(value_, LV_ALIGN_TOP_RIGHT, -77, 2);
            lv_obj_align(unit_, LV_ALIGN_TOP_LEFT, 260, 31);
        } else if (geometry.size == TileSize::Wide && !is_flag) {
            lv_obj_align(title_, LV_ALIGN_TOP_MID, 0, 7);
            lv_obj_align(value_, LV_ALIGN_TOP_MID, 0, 29);
            lv_obj_set_style_text_font(unit_, &lv_font_montserrat_12, 0);
            lv_obj_align(unit_, LV_ALIGN_TOP_RIGHT, -8, 56);
        } else if (row) {
            // Keep flag title and status side-by-side in the short analog row.
            lv_obj_set_width(title_, 136);
            lv_obj_set_style_text_align(title_, LV_TEXT_ALIGN_LEFT, 0);
            lv_obj_align(title_, LV_ALIGN_LEFT_MID, 10, 0);
            lv_obj_align(value_, LV_ALIGN_RIGHT_MID, -12, 0);
            lv_obj_set_width(unit_, 112);
            lv_obj_set_style_text_font(unit_, &lv_font_montserrat_12, 0);
            lv_obj_set_style_text_align(unit_, LV_TEXT_ALIGN_RIGHT, 0);
            lv_obj_align(unit_, LV_ALIGN_BOTTOM_RIGHT, -4, -2);
        } else if (!is_flag && geometry.height < 90) {
            lv_obj_set_style_text_align(title_, LV_TEXT_ALIGN_LEFT, 0);
            lv_obj_align(title_, LV_ALIGN_TOP_LEFT, 15, 7);
            const auto native_unit = parameterDescriptor(config.parameter).native_unit;
            const bool has_unit = native_unit != NativeUnit::None && native_unit != NativeUnit::Lambda;
            lv_obj_set_width(value_, has_unit ? 96 : geometry.width - 24);
            lv_obj_set_style_text_align(value_, LV_TEXT_ALIGN_CENTER, 0);
            lv_obj_align(value_, LV_ALIGN_TOP_LEFT, 12, 26);
            lv_obj_align(unit_, LV_ALIGN_TOP_LEFT, 108, 43);
        } else if (!is_flag) {
            lv_obj_set_width(value_, geometry.width - 24);
            lv_obj_set_style_text_align(value_, LV_TEXT_ALIGN_CENTER, 0);
            lv_obj_align(value_, LV_ALIGN_TOP_MID, 0, 31);
            lv_obj_align(unit_, LV_ALIGN_BOTTOM_MID, 0, -11);
        }
    }
    if (inline_tile && !is_flag) {
        // One line: label left, value and unit right (status bands, strips).
        lv_obj_set_style_text_font(title_, &lv_font_montserrat_12, 0);
        lv_obj_set_style_text_color(title_, UiTheme::muted(), 0);
        lv_obj_set_width(title_, geometry.width / 2);
        lv_obj_set_style_text_align(title_, LV_TEXT_ALIGN_LEFT, 0);
        lv_obj_align(title_, LV_ALIGN_LEFT_MID, 4, 0);
        lv_obj_set_width(value_, geometry.width / 2);
        lv_obj_set_style_text_align(value_, LV_TEXT_ALIGN_RIGHT, 0);
        lv_obj_align(value_, LV_ALIGN_RIGHT_MID, -44, 0);
        lv_obj_set_style_text_font(unit_, &lv_font_montserrat_12, 0);
        lv_obj_set_width(unit_, 40);
        lv_obj_set_style_text_align(unit_, LV_TEXT_ALIGN_LEFT, 0);
        lv_obj_align(unit_, LV_ALIGN_RIGHT_MID, 0, 2);
    }
    if (gauge && !is_flag) {
        lv_obj_clear_flag(gauge_, LV_OBJ_FLAG_HIDDEN);
        lv_obj_set_pos(gauge_, 8, 6);
        lv_obj_set_size(gauge_, 172, geometry.height - 12);
        lv_obj_set_width(title_, geometry.width - 196);
        lv_obj_align(title_, LV_ALIGN_TOP_RIGHT, -12, 34);
        lv_obj_set_width(value_, geometry.width - 196 - 40);
        lv_obj_set_style_text_align(value_, LV_TEXT_ALIGN_RIGHT, 0);
        lv_obj_align(value_, LV_ALIGN_TOP_RIGHT, -52, 64);
        lv_obj_set_style_text_font(unit_, &lv_font_montserrat_16, 0);
        lv_obj_align(unit_, LV_ALIGN_TOP_RIGHT, -12, 96);
        gauge_range_ = gaugeRange(config);
        lv_obj_add_flag(temperature_bar_, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(gauge_, LV_OBJ_FLAG_HIDDEN);
    }
    gauge_valid_ = false;
    gauge_fill_ = 0U;
    last_tone_ = DeltaTone::Neutral;
    display_filter_.reset();
    refresh_initialized_ = false;
    value_text_initialized_ = false;
    unit_text_initialized_ = false;
    last_row_border_ = -1;
    raw_valid_initialized_ = false;
    warning_initialized_ = false;
    temperature_bar_initialized_ = false;
    flag_style_initialized_ = false;
}
void TileView::hide() { if (root_) lv_obj_add_flag(root_, LV_OBJ_FLAG_HIDDEN); }
void TileView::arrangeUnit(const char* text, bool is_flag) {
    const bool row = size_ == TileSize::CompactRow;
    if (!is_flag && (size_ == TileSize::Inline || size_ == TileSize::Gauge)) {
        const bool status = statusText(text);
        lv_obj_set_style_text_font(unit_, status || size_ == TileSize::Inline
            ? &lv_font_montserrat_12 : &lv_font_montserrat_16, 0);
        lv_obj_set_width(unit_, status ? 120 : 40);
        lv_obj_set_style_text_align(unit_, status ? LV_TEXT_ALIGN_RIGHT
                                                  : LV_TEXT_ALIGN_LEFT, 0);
        if (size_ == TileSize::Inline)
            lv_obj_align(unit_, LV_ALIGN_RIGHT_MID, 0, 2);
        else
            lv_obj_align(unit_, LV_ALIGN_TOP_RIGHT, -12, status ? 126 : 96);
        return;
    }
    lv_obj_set_width(unit_, LV_SIZE_CONTENT);
    if (statusText(text)) {
        lv_obj_set_style_text_font(unit_, &lv_font_montserrat_12, 0);
        lv_obj_set_width(unit_, row ? 136 : geometry_.width - 24);
        lv_obj_set_style_text_align(unit_, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_align(unit_, row ? LV_ALIGN_BOTTOM_RIGHT : LV_ALIGN_BOTTOM_MID,
                     row ? -12 : 0, -2);
        if (!is_flag && !row && geometry_.height < 90) {
            lv_obj_set_width(value_, geometry_.width - 24);
            lv_obj_align(value_, LV_ALIGN_TOP_MID, 0, 28);
        }
    } else if (is_flag) {
        lv_obj_set_style_text_font(unit_, &lv_font_montserrat_12, 0);
        lv_obj_set_width(unit_, geometry_.width - 24);
        lv_obj_align(unit_, LV_ALIGN_BOTTOM_MID, 0, -2);
    } else if (size_ == TileSize::Hero || size_ == TileSize::GearHero) {
        lv_obj_set_style_text_font(unit_, &lv_font_montserrat_24, 0);
        lv_obj_align(unit_, LV_ALIGN_BOTTOM_MID, 0,
                     size_ == TileSize::GearHero ? -18 : geometry_.height > 200 ? -43 : -12);
    } else if (row) {
        lv_obj_set_style_text_font(unit_, &lv_font_montserrat_16, 0);
        lv_obj_align(unit_, LV_ALIGN_TOP_LEFT, 260, 31);
    } else if (size_ == TileSize::Wide) {
        lv_obj_set_style_text_font(unit_, &lv_font_montserrat_12, 0);
        lv_obj_align(unit_, LV_ALIGN_TOP_RIGHT, -8, 56);
    } else if (geometry_.height < 90) {
        lv_obj_set_style_text_font(unit_, &lv_font_montserrat_16, 0);
        lv_obj_align(unit_, LV_ALIGN_TOP_LEFT, 108, 43);
        lv_obj_set_width(value_, text[0] ? 96 : geometry_.width - 24);
        lv_obj_align(value_, LV_ALIGN_TOP_LEFT, 12, 26);
    } else {
        lv_obj_set_style_text_font(unit_, &lv_font_montserrat_16, 0);
        lv_obj_align(unit_, LV_ALIGN_TOP_MID, 0, geometry_.height - 27);
    }
}
void TileView::update(const TileConfig& config, const UnitSettings& units,
                      const CompositeTelemetryView& state, bool supported,
                      bool warning_active,
                      uint32_t now_ms, bool data_lost) {
    if (!root_ || lv_obj_has_flag(root_, LV_OBJ_FLAG_HIDDEN)) return;
    data_lost = data_lost && warning_active;
    const bool data_lost_changed =
        warning_initialized_ && data_lost != last_data_lost_;
    if (!warning_initialized_ || warning_active != last_warning_active_ ||
        data_lost != last_data_lost_) {
        lv_obj_set_style_border_width(root_, warning_active ? 3 : base_border_ ? 1 : 0, 0);
        lv_obj_set_style_border_color(
            root_, data_lost ? UiTheme::orange()
                   : warning_active ? UiTheme::red() : UiTheme::border(), 0);
        warning_initialized_ = true;
        last_warning_active_ = warning_active;
        last_data_lost_ = data_lost;
    }

    const SignalValue& source = state.get(config.parameter);
    const SignalValue raw = supported ? source : SignalValue{};
    const bool validity_changed = !raw_valid_initialized_ ||
                                  raw.valid != last_raw_valid_;
    const uint32_t interval_ms = tileRefreshIntervalMs(config.parameter);
    if (!refresh_initialized_) {
        next_refresh_ms_ = now_ms;
        refresh_initialized_ = true;
    }
    if (!validity_changed && !data_lost_changed &&
        static_cast<int32_t>(now_ms - next_refresh_ms_) < 0) {
        return;
    }
    if (static_cast<int32_t>(now_ms - next_refresh_ms_) >= 0) {
        const uint32_t elapsed = now_ms - next_refresh_ms_;
        next_refresh_ms_ += (elapsed / interval_ms + 1U) * interval_ms;
    }
    raw_valid_initialized_ = true;
    last_raw_valid_ = raw.valid;

    const bool is_flag = parameterDescriptor(config.parameter).kind ==
                         ParameterKind::Flag;
    const SignalValue signal = is_flag
        ? raw
        : display_filter_.sample(raw, now_ms, interval_ms * 2U);
    const TemperatureBarState temperature = temperatureBarState(
        config.parameter, signal, config.temperature_bar);
    const bool bar_visible = temperature.visible && supported;
    if (bar_visible == lv_obj_has_flag(temperature_bar_, LV_OBJ_FLAG_HIDDEN)) {
        if (bar_visible) lv_obj_clear_flag(temperature_bar_, LV_OBJ_FLAG_HIDDEN);
        else lv_obj_add_flag(temperature_bar_, LV_OBJ_FLAG_HIDDEN);
    }
    if (temperature.visible &&
        (!temperature_bar_initialized_ ||
         temperature.fill_per_mille != last_temperature_fill_ ||
         temperature.zone != last_temperature_zone_)) {
        lv_bar_set_value(
            temperature_bar_, temperature.fill_per_mille, LV_ANIM_OFF);
        lv_obj_set_style_bg_color(
            temperature_bar_, temperatureBarColor(temperature.zone),
            LV_PART_INDICATOR);
        last_temperature_fill_ = temperature.fill_per_mille;
        last_temperature_zone_ = temperature.zone;
        temperature_bar_initialized_ = true;
    }
    char value_text[32] = "---";
    const char* unit_text = "";
    if (is_flag) {
        const FlagTilePresentation presentation = flagTilePresentation(
            signal, config.flag_active_color);
        // Restyling every refresh forces LVGL to recompute styles and
        // invalidate the tile; only touch styles when the look changes.
        const bool style_changed = !flag_style_initialized_ ||
            presentation.state != last_flag_state_ ||
            presentation.background_rgb != last_flag_background_ ||
            presentation.pill_rgb != last_flag_pill_;
        flag_style_initialized_ = true;
        last_flag_state_ = presentation.state;
        last_flag_background_ = presentation.background_rgb;
        last_flag_pill_ = presentation.pill_rgb;
        if (presentation.state != FlagTileState::Unavailable)
            std::snprintf(value_text, sizeof(value_text), "%s",
                          presentation.status_text);
        else
            unit_text = "UNAVAILABLE";
        if (style_changed) {
            lv_obj_set_style_bg_color(
                root_, lv_color_hex(presentation.background_rgb), 0);
            lv_obj_set_style_bg_color(
                stripe_, UiTheme::border(), 0);
            if (presentation.state == FlagTileState::Unavailable) {
                lv_obj_set_style_bg_opa(value_, LV_OPA_TRANSP, 0);
                lv_obj_set_style_pad_all(value_,0,0);
                lv_obj_set_style_text_color(value_, UiTheme::muted(), 0);
            } else {
                lv_obj_set_style_bg_opa(value_, LV_OPA_COVER, 0);
                lv_obj_set_style_bg_color(
                    value_, lv_color_hex(presentation.pill_rgb), 0);
                lv_obj_set_style_radius(value_, 14, 0);
                lv_obj_set_style_pad_left(value_, 12, 0);
                lv_obj_set_style_pad_right(value_, 12, 0);
                lv_obj_set_style_pad_top(value_, 4, 0);
                lv_obj_set_style_pad_bottom(value_, 4, 0);
                lv_obj_set_style_text_color(value_, UiTheme::background(), 0);
            }
        }
    } else if (!supported) {
        unit_text = "UNAVAILABLE";
    } else if (!signal.valid && data_lost) {
        unit_text = "DATA LOST";
    } else if (signal.valid) {
        const PresentedValue p = UnitPresenter::present(
            config.parameter, signal.value, units);
        formatTileValue(config.parameter, p.value, config.decimals,
                        value_text, sizeof(value_text));
        unit_text = std::strcmp(p.unit,"lambda") == 0 ? "" : p.unit;
        if (std::strchr(value_text, ':') != nullptr) unit_text = "";
        const DeltaTone tone = deltaTone(config.parameter, p.value);
        if (tone != last_tone_) {
            last_tone_ = tone;
            lv_obj_set_style_text_color(value_, tone == DeltaTone::Better ? UiTheme::green()
                : tone == DeltaTone::Worse ? UiTheme::yellow() : UiTheme::text(), 0);
        }
        if (size_ == TileSize::Hero && config.parameter == ParameterId::Rpm) unit_text = "RPM";
    }

    if (size_ == TileSize::Gauge && !is_flag) {
        const bool gauge_valid = supported && signal.valid;
        const uint16_t fill = gauge_valid ? gaugeFill(signal.value, gauge_range_) : 0U;
        if (gauge_valid != gauge_valid_ || fill != gauge_fill_) {
            gauge_valid_ = gauge_valid;
            gauge_fill_ = fill;
            lv_obj_invalidate(gauge_);
        }
    }
    const bool row_changed = !value_text_initialized_ || !unit_text_initialized_ ||
        std::strcmp(last_value_text_.data(), value_text) != 0 ||
        std::strcmp(last_unit_text_.data(), unit_text) != 0 ||
        last_row_border_ != lv_obj_get_style_border_width(root_,0);
    if (!unit_text_initialized_ || std::strcmp(last_unit_text_.data(), unit_text) != 0)
        arrangeUnit(unit_text, is_flag);
    if (!value_text_initialized_ ||
        std::strcmp(last_value_text_.data(), value_text) != 0) {
        if (!is_flag) {
            // Fit the actual presented text, including decimals and unit conversion.
            const lv_font_t* candidates[]={default_value_font_,&race_digits_96,&race_digits_64,
                &race_digits_48,&lv_font_montserrat_24,&lv_font_montserrat_16};
            const lv_font_t* fitted=&lv_font_montserrat_16;
            for (const auto* font:candidates) {
                if (!supported && font->line_height > lv_font_montserrat_24.line_height) continue;
                if (font->line_height > default_value_font_->line_height) continue;
                lv_point_t measured;
                lv_txt_get_size(&measured,value_text,font,0,0,LV_COORD_MAX,LV_TEXT_FLAG_NONE);
                const int capacity=size_==TileSize::CompactRow ? 123:lv_obj_get_width(value_);
                if (measured.x <= capacity) { fitted=font; break; }
            }
            if (lv_obj_get_style_text_font(value_,LV_PART_MAIN) != fitted)
                lv_obj_set_style_text_font(value_,fitted,0);
        }
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
    if(row_changed)arrangeRow(is_flag);
}
TileAddress TileView::address() const { return address_; }
void TileView::arrangeRow(bool is_flag) {
    if(size_!=TileSize::CompactRow || is_flag)return;
    const int border=lv_obj_get_style_border_width(root_,0);
    last_row_border_=border;
    const int center=geometry_.height/2-border;
    lv_obj_set_width(title_,120);
    lv_obj_set_style_text_align(title_,LV_TEXT_ALIGN_CENTER,0);
    lv_obj_set_style_text_color(title_,UiTheme::muted(),0);
    lv_obj_align(title_,LV_ALIGN_TOP_LEFT,12-border,opticalTextY(lv_label_get_text(title_),&lv_font_montserrat_16,center));
    const auto* font=lv_obj_get_style_text_font(value_,0);
    lv_point_t v,u;
    lv_txt_get_size(&v,lv_label_get_text(value_),font,0,0,LV_COORD_MAX,LV_TEXT_FLAG_NONE);
    const bool unavailable=statusText(lv_label_get_text(unit_));
    lv_txt_get_size(&u,lv_label_get_text(unit_),&lv_font_montserrat_16,0,0,LV_COORD_MAX,LV_TEXT_FLAG_NONE);
    const int gap=u.x && !unavailable ? 8:0;
    const int group_width=v.x+gap+(unavailable ? 0:u.x);
    const int x=228-border-group_width/2;
    lv_obj_set_width(value_,v.x+1);
    lv_obj_set_style_text_align(value_,LV_TEXT_ALIGN_LEFT,0);
    lv_obj_align(value_,LV_ALIGN_TOP_LEFT,x,opticalTextY(lv_label_get_text(value_),font,center-(unavailable ? 7:0)));
    if(!unavailable) {
        lv_obj_set_width(unit_,u.x+1);
        lv_obj_align(unit_,LV_ALIGN_TOP_LEFT,x+v.x+gap,opticalTextY(lv_label_get_text(unit_),&lv_font_montserrat_16,center));
    }
}

namespace {
constexpr float kGaugePi = 3.14159265358979323846f;
lv_point_t gaugePoint(lv_point_t center, float radius, float degrees) {
    const float a = degrees * kGaugePi / 180.0f;
    return {static_cast<lv_coord_t>(std::lround(center.x + radius * std::cos(a))),
            static_cast<lv_coord_t>(std::lround(center.y + radius * std::sin(a)))};
}
}  // namespace

// Mini needle gauge: 240 degree segmented arc, lit up to the value, red zone
// from the tile's warning threshold or temperature red mark.
void TileView::drawGauge(lv_event_t* event) {
    auto* self = static_cast<TileView*>(lv_event_get_user_data(event));
    if (self == nullptr || self->gauge_ == nullptr) return;
    lv_draw_ctx_t* ctx = lv_event_get_draw_ctx(event);
    lv_area_t area;
    lv_obj_get_coords(self->gauge_, &area);
    const int width = area.x2 - area.x1 + 1;
    const int height = area.y2 - area.y1 + 1;
    const int radius = std::min(width / 2 - 14, height / 2 - 4);
    const lv_point_t center{static_cast<lv_coord_t>(area.x1 + width / 2),
                            static_cast<lv_coord_t>(area.y1 + height / 2 + 8)};
    constexpr int kSegments = 24;
    constexpr float kStart = 150.0f, kSweep = 240.0f;
    const lv_color_t dim = lv_color_hex(0x1C2228);
    for (int i = 0; i < kSegments; ++i) {
        const uint16_t at = static_cast<uint16_t>((i * 1000 + 500) / kSegments);
        const bool red = at >= self->gauge_range_.red_from_permille ||
                         at < self->gauge_range_.red_below_permille;
        const bool lit = self->gauge_valid_ && at <= self->gauge_fill_;
        lv_draw_arc_dsc_t d;
        lv_draw_arc_dsc_init(&d);
        d.width = 12;
        d.color = lit ? (red ? UiTheme::red() : UiTheme::text()) : dim;
        const float a0 = kStart + kSweep * i / kSegments;
        const float a1 = kStart + kSweep * (i + 1) / kSegments - 2.0f;
        lv_draw_arc(ctx, &d, &center, static_cast<uint16_t>(radius),
                    static_cast<uint16_t>(std::lround(a0)) % 360U,
                    static_cast<uint16_t>(std::lround(a1)) % 360U);
    }
    // Red zone marks just outside the segments.
    lv_draw_arc_dsc_t zone;
    lv_draw_arc_dsc_init(&zone);
    zone.width = 2;
    zone.color = UiTheme::red();
    if (self->gauge_range_.red_from_permille <= 1000U) {
        const float a0 = kStart + kSweep * self->gauge_range_.red_from_permille / 1000.0f;
        lv_draw_arc(ctx, &zone, &center, static_cast<uint16_t>(radius + 6),
                    static_cast<uint16_t>(std::lround(a0)) % 360U,
                    static_cast<uint16_t>(std::lround(kStart + kSweep)) % 360U);
    }
    if (self->gauge_range_.red_below_permille > 0U) {
        const float a1 = kStart + kSweep * self->gauge_range_.red_below_permille / 1000.0f;
        lv_draw_arc(ctx, &zone, &center, static_cast<uint16_t>(radius + 6),
                    static_cast<uint16_t>(kStart),
                    static_cast<uint16_t>(std::lround(a1)) % 360U);
    }
    if (!self->gauge_valid_) return;
    const float angle = kStart + kSweep * self->gauge_fill_ / 1000.0f;
    lv_draw_line_dsc_t needle;
    lv_draw_line_dsc_init(&needle);
    needle.color = UiTheme::text();
    needle.width = 3;
    needle.round_start = needle.round_end = 1;
    const lv_point_t tip = gaugePoint(center, radius + 8, angle);
    const lv_point_t tail = gaugePoint(center, radius - 26, angle);
    lv_draw_line(ctx, &needle, &tail, &tip);
}
