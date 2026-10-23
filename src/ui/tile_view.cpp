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
    lv_obj_add_event_cb(root_, drawChamfer, LV_EVENT_DRAW_MAIN, this);
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
    chamfer_ = geometry.chamfer;
    styled_ = chamfer_ != 0U || geometry.size == TileSize::DialCenter ||
              geometry.size == TileSize::DialBadge;
    base_border_ = geometry.size != TileSize::Hero && !inline_tile && !styled_;
    lv_obj_set_style_text_letter_space(title_, 0, 0);
    lv_obj_set_style_text_letter_space(unit_, 0, 0);
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
    if (hero || inline_tile || styled_) lv_obj_add_flag(stripe_, LV_OBJ_FLAG_HIDDEN);
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
        const bool hide_title = !gear &&
            (config.parameter == ParameterId::Rpm || config.parameter == ParameterId::Speed ||
             geometry.height < 110);
        lv_obj_align(value_, LV_ALIGN_TOP_MID, 0,
                     gear ? (compact ? 34 : 100)
                          : geometry.height > 200 ? 60 : hide_title ? 10 : 30);
        lv_obj_align(title_, LV_ALIGN_TOP_MID, 0, gear ? (compact ? 4 : 26) : 2);
        if (gear && compact) lv_obj_set_style_text_font(title_, &lv_font_montserrat_16, 0);
        if (hide_title) lv_obj_add_flag(title_, LV_OBJ_FLAG_HIDDEN);
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
            const int unit_x = std::min(108, geometry.width - 34);
            lv_obj_set_width(value_, has_unit ? unit_x - 12 : geometry.width - 24);
            lv_obj_set_style_text_align(value_, LV_TEXT_ALIGN_CENTER, 0);
            lv_obj_align(value_, LV_ALIGN_TOP_LEFT, 12, 26);
            lv_obj_align(unit_, LV_ALIGN_TOP_LEFT, unit_x, 43);
        } else if (!is_flag) {
            lv_obj_set_width(value_, geometry.width - 24);
            lv_obj_set_style_text_align(value_, LV_TEXT_ALIGN_CENTER, 0);
            lv_obj_align(value_, LV_ALIGN_TOP_MID, 0, 31);
            lv_obj_align(unit_, LV_ALIGN_BOTTOM_MID, 0, -11);
        }
    }
    if (inline_tile && !is_flag) {
        // Short: one line, label left and value right (status band rows).
        // Tall: label on top, value below right-aligned (strips).
        const bool stacked = geometry.height >= 50;
        lv_obj_set_style_text_font(title_, &lv_font_montserrat_12, 0);
        lv_obj_set_style_text_color(title_, UiTheme::muted(), 0);
        lv_obj_set_width(title_, stacked ? geometry.width - 8 : geometry.width * 2 / 5);
        lv_obj_set_style_text_align(title_, LV_TEXT_ALIGN_LEFT, 0);
        if (stacked) lv_obj_align(title_, LV_ALIGN_TOP_LEFT, 4, 2);
        else lv_obj_align(title_, LV_ALIGN_LEFT_MID, 4, 0);
        lv_obj_set_width(value_, stacked ? geometry.width - 52
                                         : geometry.width * 3 / 5 - 48);
        lv_obj_set_style_text_align(value_, LV_TEXT_ALIGN_RIGHT, 0);
        lv_obj_align(value_, stacked ? LV_ALIGN_BOTTOM_RIGHT : LV_ALIGN_RIGHT_MID,
                     -44, stacked ? 2 : 0);
        lv_obj_set_style_text_font(unit_, &lv_font_montserrat_12, 0);
        lv_obj_set_width(unit_, 40);
        lv_obj_set_style_text_align(unit_, LV_TEXT_ALIGN_LEFT, 0);
        lv_obj_align(unit_, LV_ALIGN_RIGHT_MID, 0, 2);
    }
    if (gauge && !is_flag) {
        lv_obj_clear_flag(gauge_, LV_OBJ_FLAG_HIDDEN);
        lv_obj_set_pos(gauge_, 1, (geometry.height - 150) / 2);
        lv_obj_set_size(gauge_, 192, 150);
        lv_obj_set_width(title_, geometry.width - 196);
        lv_obj_align(title_, LV_ALIGN_TOP_RIGHT, -12, 34);
        lv_obj_set_width(value_, geometry.width - 196 - 40);
        lv_obj_set_style_text_align(value_, LV_TEXT_ALIGN_RIGHT, 0);
        lv_obj_align(value_, LV_ALIGN_TOP_RIGHT, -52, 64);
        lv_obj_set_style_text_font(unit_, &lv_font_montserrat_16, 0);
        lv_obj_align(unit_, LV_ALIGN_TOP_RIGHT, -12, 96);
        gauge_range_ = gaugeRange(config);
        gauge_celsius_ = parameterDescriptor(config.parameter).native_unit == NativeUnit::Celsius;
        lv_obj_add_flag(temperature_bar_, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(gauge_, LV_OBJ_FLAG_HIDDEN);
    }
    if (styled_) setupStyled(config);
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
        if (size_ == TileSize::Inline) {
            lv_obj_align(unit_, LV_ALIGN_RIGHT_MID, 0, 2);
            // Times carry no unit: give the value the unit's room too.
            const bool stacked = geometry_.height >= 50;
            const int reserve = text[0] ? 44 : 4;
            lv_obj_set_width(value_, (stacked ? geometry_.width - 8 : geometry_.width * 3 / 5 - 4) - reserve);
            lv_obj_align(value_, stacked ? LV_ALIGN_BOTTOM_RIGHT : LV_ALIGN_RIGHT_MID,
                         -reserve, stacked ? 2 : 0);
        } else {
            lv_obj_align(unit_, LV_ALIGN_TOP_RIGHT, -12, status ? 126 : 96);
        }
        return;
    }
    if (!is_flag && !text[0] && (size_ == TileSize::Wide || size_ == TileSize::Large ||
                                 (size_ == TileSize::Small && geometry_.height >= 90))) {
        lv_obj_set_width(value_, geometry_.width - 24);
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
        const int unit_x = std::min(108, geometry_.width - 34);
        if (geometry_.width < 150) lv_obj_set_style_text_font(unit_, &lv_font_montserrat_12, 0);
        lv_obj_align(unit_, LV_ALIGN_TOP_LEFT, unit_x, 43);
        lv_obj_set_width(value_, text[0] ? unit_x - 12 : geometry_.width - 24);
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
        styled_warning_ = warning_active;
        styled_data_lost_ = data_lost;
        if (chamfer_ != 0U) lv_obj_invalidate(root_);
        lv_obj_set_style_border_width(root_, chamfer_ != 0U ? 0
            : warning_active ? 3 : base_border_ ? 1 : 0, 0);
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
    const bool bar_visible = temperature.visible && supported &&
                             size_ != TileSize::Gauge && size_ != TileSize::Inline;
    if (bar_visible == lv_obj_has_flag(temperature_bar_, LV_OBJ_FLAG_HIDDEN)) {
        if (bar_visible) lv_obj_clear_flag(temperature_bar_, LV_OBJ_FLAG_HIDDEN);
        else lv_obj_add_flag(temperature_bar_, LV_OBJ_FLAG_HIDDEN);
        if (styled_ && value_text_initialized_) arrangeStyled();
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
        const PresentedValue low = UnitPresenter::present(config.parameter, gauge_range_.minimum, units);
        const PresentedValue high = UnitPresenter::present(config.parameter, gauge_range_.maximum, units);
        char unit_label[8];
        std::snprintf(unit_label, sizeof(unit_label), "%s", high.unit);
        for (char* c = unit_label; *c; ++c) if (*c >= 'a' && *c <= 'z') *c = static_cast<char>(*c - 32);
        if (low.value != gauge_label_min_ || high.value != gauge_label_max_ ||
            std::strcmp(unit_label, gauge_unit_.data()) != 0) {
            gauge_label_min_ = low.value;
            gauge_label_max_ = high.value;
            std::snprintf(gauge_unit_.data(), gauge_unit_.size(), "%s", unit_label);
            lv_obj_invalidate(gauge_);
        }
        if (gauge_valid != gauge_valid_ || fill != gauge_fill_) {
            gauge_valid_ = gauge_valid;
            gauge_fill_ = fill;
            lv_obj_invalidate(gauge_);
        }
    }
    if (styled_) {
        const bool changed = !value_text_initialized_ || !unit_text_initialized_ ||
            std::strcmp(last_value_text_.data(), value_text) != 0 ||
            std::strcmp(last_unit_text_.data(), unit_text) != 0;
        if (changed) {
            lv_label_set_text(value_, value_text);
            lv_label_set_text(unit_, unit_text);
            std::snprintf(last_value_text_.data(), last_value_text_.size(), "%s", value_text);
            std::snprintf(last_unit_text_.data(), last_unit_text_.size(), "%s", unit_text);
            value_text_initialized_ = unit_text_initialized_ = true;
            arrangeStyled();
        }
        return;
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
                // The style width is current even before LVGL recomputes
                // the layout (first frame, freshly re-arranged unit).
                const lv_coord_t styled=lv_obj_get_style_width(value_,LV_PART_MAIN);
                const int capacity=size_==TileSize::CompactRow ? geometry_.width-205
                    : (styled>0 && !LV_COORD_IS_SPEC(styled)) ? styled
                    : lv_obj_get_width(value_);
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

// Mini needle gauge (ANALOG TWIN): 240 degree segmented arc lit up to the
// value, thin inner trail, scale numbers, red zone outside, white needle.
void TileView::drawGauge(lv_event_t* event) {
    auto* self = static_cast<TileView*>(lv_event_get_user_data(event));
    if (self == nullptr || self->gauge_ == nullptr) return;
    lv_draw_ctx_t* ctx = lv_event_get_draw_ctx(event);
    lv_area_t area;
    lv_obj_get_coords(self->gauge_, &area);
    const lv_point_t c{static_cast<lv_coord_t>(area.x1 + 96), static_cast<lv_coord_t>(area.y1 + 92)};
    constexpr float kStart = 150.0f, kSweep = 240.0f;
    constexpr int kSegments = 24;
    const GaugeRange& range = self->gauge_range_;
    const bool fill_red = self->gauge_valid_ &&
        (self->gauge_fill_ >= range.red_from_permille || self->gauge_fill_ < range.red_below_permille);
    const lv_color_t lit = fill_red ? UiTheme::red()
                         : self->gauge_celsius_ ? lv_color_hex(0x22C55E) : lv_color_hex(0xE6EAEE);
    const auto arcDeg = [&](int radius, float from, float to, lv_color_t color, int width, lv_opa_t opa) {
        if (to <= from) return;
        lv_draw_arc_dsc_t d;
        lv_draw_arc_dsc_init(&d);
        d.color = color;
        d.width = static_cast<lv_coord_t>(width);
        d.opa = opa;
        lv_draw_arc(ctx, &d, &c, static_cast<uint16_t>(radius),
                    static_cast<uint16_t>(std::lround(from)) % 360U,
                    static_cast<uint16_t>(std::lround(to)) % 360U);
    };
    // Red zone(s) outside the segments.
    if (range.red_from_permille <= 1000U)
        arcDeg(72, kStart + kSweep * range.red_from_permille / 1000.0f, kStart + kSweep,
               UiTheme::red(), 3, LV_OPA_COVER);
    if (range.red_below_permille > 0U)
        arcDeg(72, kStart, kStart + kSweep * range.red_below_permille / 1000.0f,
               UiTheme::red(), 3, LV_OPA_COVER);
    for (int i = 0; i < kSegments; ++i) {
        const float a0 = kStart + kSweep * i / kSegments;
        const uint16_t at = static_cast<uint16_t>((i * 1000 + 500) / kSegments);
        const bool on = self->gauge_valid_ && at <= self->gauge_fill_;
        arcDeg(58, a0, a0 + 7.0f, on ? lit : lv_color_hex(0x1A2129), 12, LV_OPA_COVER);
    }
    if (self->gauge_valid_)
        arcDeg(47, kStart, kStart + kSweep * self->gauge_fill_ / 1000.0f, lit, 2, LV_OPA_50);
    // Scale numbers at 0/25/50/75/100 %.
    lv_draw_label_dsc_t number;
    lv_draw_label_dsc_init(&number);
    number.font = &dash_italic_14;
    number.color = lv_color_hex(0x8E9AA6);
    number.align = LV_TEXT_ALIGN_CENTER;
    for (int k = 0; k <= 4; ++k) {
        const float value = self->gauge_label_min_ + (self->gauge_label_max_ - self->gauge_label_min_) * k / 4.0f;
        char text[8];
        std::snprintf(text, sizeof(text), "%.0f", static_cast<double>(value));
        const lv_point_t p = gaugePoint(c, 40, kStart + kSweep * k / 4.0f);
        const lv_area_t box{static_cast<lv_coord_t>(p.x - 16), static_cast<lv_coord_t>(p.y - 8),
                            static_cast<lv_coord_t>(p.x + 16), static_cast<lv_coord_t>(p.y + 9)};
        lv_draw_label(ctx, &number, &box, text, nullptr);
    }
    lv_draw_label_dsc_t unit;
    lv_draw_label_dsc_init(&unit);
    unit.font = &dash_label_12;
    unit.color = lv_color_hex(0x5E6872);
    unit.letter_space = 2;
    unit.align = LV_TEXT_ALIGN_CENTER;
    const lv_area_t unit_box{static_cast<lv_coord_t>(c.x - 30), static_cast<lv_coord_t>(c.y + 26),
                             static_cast<lv_coord_t>(c.x + 30), static_cast<lv_coord_t>(c.y + 42)};
    lv_draw_label(ctx, &unit, &unit_box, self->gauge_unit_.data(), nullptr);
    if (!self->gauge_valid_) return;
    // Needle: a slim white wedge from just inside the segments to the red zone.
    const float angle = kStart + kSweep * self->gauge_fill_ / 1000.0f;
    const float radians = angle * kGaugePi / 180.0f;
    const float nx = -std::sin(radians), ny = std::cos(radians);
    const lv_point_t base = gaugePoint(c, 49, angle), tip = gaugePoint(c, 73, angle);
    const lv_point_t needle[4] = {
        {static_cast<lv_coord_t>(std::lround(base.x + nx * 3)), static_cast<lv_coord_t>(std::lround(base.y + ny * 3))},
        {static_cast<lv_coord_t>(std::lround(tip.x + nx * 1)), static_cast<lv_coord_t>(std::lround(tip.y + ny * 1))},
        {static_cast<lv_coord_t>(std::lround(tip.x - nx * 1)), static_cast<lv_coord_t>(std::lround(tip.y - ny * 1))},
        {static_cast<lv_coord_t>(std::lround(base.x - nx * 3)), static_cast<lv_coord_t>(std::lround(base.y - ny * 3))}};
    lv_draw_rect_dsc_t white;
    lv_draw_rect_dsc_init(&white);
    white.bg_color = lv_color_hex(0xFFFFFF);
    lv_draw_polygon(ctx, &white, needle, 4);
}

namespace {
const lv_color_t kStyledLabel = lv_color_hex(0x8E9AA6);
int textWidth(const char* text, const lv_font_t* font, int letter_space = 0) {
    lv_point_t size;
    lv_txt_get_size(&size, text, font, letter_space, 0, LV_COORD_MAX, LV_TEXT_FLAG_NONE);
    return size.x;
}
// Places a label so that its text baseline lands on `baseline`.
void placeOnBaseline(lv_obj_t* label, const lv_font_t* font, int x, int baseline) {
    lv_obj_align(label, LV_ALIGN_TOP_LEFT, x, baseline - (font->line_height - font->base_line));
}
int capHeight(const lv_font_t* font) {
    return font->line_height - font->base_line;
}
}  // namespace

void TileView::setupStyled(const TileConfig&) {
    lv_obj_set_style_bg_opa(root_, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(root_, 0, 0);
    lv_obj_set_style_radius(root_, 0, 0);
    lv_obj_set_style_text_font(title_, &dash_label_12, 0);
    lv_obj_set_style_text_letter_space(title_, 2, 0);
    lv_obj_set_style_text_color(title_, kStyledLabel, 0);
    lv_obj_set_width(title_, LV_SIZE_CONTENT);
    lv_obj_set_width(value_, LV_SIZE_CONTENT);
    lv_obj_set_width(unit_, LV_SIZE_CONTENT);
    lv_label_set_long_mode(value_, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_align(value_, LV_TEXT_ALIGN_LEFT, 0);
    lv_obj_set_style_text_font(unit_, &dash_label_14, 0);
    lv_obj_set_style_text_color(unit_, kStyledLabel, 0);
    switch (size_) {
        case TileSize::DialCenter: styled_value_font_ = &dash_italic_80; break;
        case TileSize::Inline: styled_value_font_ = &dash_italic_38; break;
        default: styled_value_font_ = &dash_italic_48; break;
    }
    lv_obj_set_style_text_font(value_, styled_value_font_, 0);
    if (size_ == TileSize::DialCenter) {
        lv_obj_set_style_text_letter_space(unit_, 2, 0);
        lv_obj_set_style_text_letter_space(title_, 2, 0);
    }
    if (size_ == TileSize::DialBadge) {
        lv_obj_add_flag(title_, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(unit_, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_clear_flag(title_, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(unit_, LV_OBJ_FLAG_HIDDEN);
    }
    lv_obj_set_style_radius(temperature_bar_, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(temperature_bar_, 0, LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(temperature_bar_, lv_color_hex(0x1C2228), LV_PART_MAIN);
    arrangeStyled();
}

void TileView::arrangeStyled() {
    const int w = geometry_.width;
    const int h = geometry_.height;
    const char* value = lv_label_get_text(value_);
    const char* unit = lv_label_get_text(unit_);
    const bool status = statusText(unit);
    const lv_font_t* vf = styled_value_font_ ? styled_value_font_ : &dash_italic_48;
    // Fall back to a smaller italic size when the text does not fit.
    const int room = size_ == TileSize::Gauge ? 140 : w - 20;
    if (textWidth(value, vf) > room && vf != &dash_italic_38 && size_ != TileSize::DialCenter) vf = &dash_italic_38;
    if (lv_obj_get_style_text_font(value_, LV_PART_MAIN) != vf) lv_obj_set_style_text_font(value_, vf, 0);
    const lv_font_t* uf = &dash_label_14;
    const lv_font_t* tf = &dash_label_12;
    const int vw = textWidth(value, vf);
    const int uw = unit[0] ? textWidth(unit, uf, size_ == TileSize::DialCenter ? 2 : 0) : 0;
    const int tw = textWidth(lv_label_get_text(title_), tf, 2);
    const int gap = unit[0] && !status ? 5 : 0;
    const int cap = capHeight(vf);
    if (status) lv_obj_set_style_text_color(unit_, UiTheme::orange(), 0);
    else lv_obj_set_style_text_color(unit_, kStyledLabel, 0);

    if (size_ == TileSize::DialCenter) {
        const int baseline = 62;
        placeOnBaseline(value_, vf, (w - vw) / 2, baseline);
        const bool show_unit = unit[0] != '\0';
        if (show_unit) {
            // Dial centre shows the unit in capitals (KM/H).
            char upper[16];
            std::snprintf(upper, sizeof(upper), "%s", unit);
            for (char* ch = upper; *ch; ++ch) if (*ch >= 'a' && *ch <= 'z') *ch = static_cast<char>(*ch - 32);
            if (std::strcmp(upper, unit) != 0) lv_label_set_text(unit_, upper);
            lv_obj_add_flag(title_, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_flag(unit_, LV_OBJ_FLAG_HIDDEN);
            placeOnBaseline(unit_, uf, (w - uw) / 2, 94);
        } else {
            lv_obj_add_flag(unit_, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_flag(title_, LV_OBJ_FLAG_HIDDEN);
            placeOnBaseline(title_, tf, (w - tw) / 2, 88);
        }
        return;
    }
    if (size_ == TileSize::DialBadge) {
        placeOnBaseline(value_, vf, (w - vw) / 2, (h + cap) / 2 - 2);
        return;
    }
    if (size_ == TileSize::Inline) {
        const int total = tw + 10 + vw + (status ? 0 : gap + uw);
        const int x0 = std::max(4, (w - total) / 2);
        const int baseline = (h + cap) / 2;
        placeOnBaseline(title_, tf, x0, h / 2 + capHeight(tf) / 2);
        placeOnBaseline(value_, vf, x0 + tw + 10, baseline);
        if (status) placeOnBaseline(unit_, &dash_label_12, (w - uw) / 2, h - 6);
        else placeOnBaseline(unit_, uf, x0 + tw + 10 + vw + gap, baseline);
        return;
    }
    const bool bar = !lv_obj_has_flag(temperature_bar_, LV_OBJ_FLAG_HIDDEN);
    const int cx = size_ == TileSize::Gauge ? w - 87 : w / 2;
    const int title_h = capHeight(tf);
    const int block = title_h + 10 + cap + (bar ? 12 : 0);
    const int top = size_ == TileSize::Gauge ? h / 2 - block / 2 - 4 : (h - block) / 2;
    placeOnBaseline(title_, tf, cx - tw / 2, top + title_h);
    const int baseline = top + title_h + 10 + cap;
    const int row = vw + (status ? 0 : gap + uw);
    const int x0 = cx - row / 2;
    placeOnBaseline(value_, vf, x0, baseline);
    if (status) placeOnBaseline(unit_, &dash_label_12, cx - uw / 2, baseline + 16);
    else placeOnBaseline(unit_, uf, x0 + vw + gap, baseline);
    if (bar) {
        const int bw = w * 62 / 100;
        lv_obj_set_size(temperature_bar_, bw, 3);
        lv_obj_align(temperature_bar_, LV_ALIGN_TOP_LEFT, cx - bw / 2, baseline + 9);
    }
}

// Chamfered panel: grey 1 px outline with a dark vertical gradient inside;
// red (warning) or orange (data lost) outline when highlighted.
void TileView::drawChamfer(lv_event_t* event) {
    auto* self = static_cast<TileView*>(lv_event_get_user_data(event));
    if (self == nullptr || self->chamfer_ == 0U) return;
    lv_draw_ctx_t* ctx = lv_event_get_draw_ctx(event);
    lv_area_t a;
    lv_obj_get_coords(self->root_, &a);
    const bool left = self->chamfer_ == 1U;
    const auto shape = [&](int inset, int cut, lv_point_t* p) {
        const int x1 = a.x1 + inset, y1 = a.y1 + inset, x2 = a.x2 - inset, y2 = a.y2 - inset;
        if (left) {  // cuts at top-left and bottom-right
            p[0] = {static_cast<lv_coord_t>(x1 + cut), static_cast<lv_coord_t>(y1)};
            p[1] = {static_cast<lv_coord_t>(x2), static_cast<lv_coord_t>(y1)};
            p[2] = {static_cast<lv_coord_t>(x2), static_cast<lv_coord_t>(y2 - cut)};
            p[3] = {static_cast<lv_coord_t>(x2 - cut), static_cast<lv_coord_t>(y2)};
            p[4] = {static_cast<lv_coord_t>(x1), static_cast<lv_coord_t>(y2)};
            p[5] = {static_cast<lv_coord_t>(x1), static_cast<lv_coord_t>(y1 + cut)};
        } else {     // cuts at top-right and bottom-left
            p[0] = {static_cast<lv_coord_t>(x1), static_cast<lv_coord_t>(y1)};
            p[1] = {static_cast<lv_coord_t>(x2 - cut), static_cast<lv_coord_t>(y1)};
            p[2] = {static_cast<lv_coord_t>(x2), static_cast<lv_coord_t>(y1 + cut)};
            p[3] = {static_cast<lv_coord_t>(x2), static_cast<lv_coord_t>(y2)};
            p[4] = {static_cast<lv_coord_t>(x1 + cut), static_cast<lv_coord_t>(y2)};
            p[5] = {static_cast<lv_coord_t>(x1), static_cast<lv_coord_t>(y2 - cut)};
        }
    };
    const int stroke = self->styled_warning_ ? 3 : 1;
    lv_point_t outer[6], inner[6];
    shape(0, 18, outer);
    shape(stroke, 18 - stroke / 2, inner);
    lv_draw_rect_dsc_t edge;
    lv_draw_rect_dsc_init(&edge);
    edge.bg_color = self->styled_data_lost_ ? UiTheme::orange()
                  : self->styled_warning_ ? UiTheme::red() : lv_color_hex(0x3A4048);
    lv_draw_polygon(ctx, &edge, outer, 6);
    lv_draw_rect_dsc_t fill;
    lv_draw_rect_dsc_init(&fill);
    fill.bg_grad.dir = LV_GRAD_DIR_VER;
    fill.bg_grad.stops_count = 2;
    fill.bg_grad.stops[0].color = lv_color_hex(0x0B1015);
    fill.bg_grad.stops[0].frac = 0;
    fill.bg_grad.stops[1].color = lv_color_hex(0x030507);
    fill.bg_grad.stops[1].frac = 255;
    lv_draw_polygon(ctx, &fill, inner, 6);
}
