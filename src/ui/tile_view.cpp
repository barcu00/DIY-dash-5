#include "tile_view.h"
#include <cstdio>
#include <cstring>
#include "telemetry/parameter_registry.h"
#include "ui/flag_tile_model.h"
#include "ui/tile_refresh_policy.h"
#include "ui/ui_theme.h"
#include "ui/unit_presenter.h"
#include "ui/tile_view_policy.h"
#include "ui/fonts/numeric_fonts.h"
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
    lv_obj_set_style_bg_color(stripe_, UiTheme::blue(), 0);
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
}
void TileView::apply(const TileConfig& config, const TileGeometry& geometry) {
    const bool hero = geometry.size == TileSize::Hero || geometry.size == TileSize::GearHero;
    const bool row = geometry.size == TileSize::CompactRow;
    const bool centered = geometry.size == TileSize::Wide || hero ||
                          geometry.size == TileSize::CenteredSmall;
    const bool is_flag = parameterDescriptor(config.parameter).kind ==
                         ParameterKind::Flag;
    size_ = geometry.size;
    geometry_ = geometry;
    base_border_ = geometry.size != TileSize::Hero;
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
    lv_obj_set_style_bg_color(stripe_, UiTheme::blue(), 0);
    if (hero) lv_obj_add_flag(stripe_, LV_OBJ_FLAG_HIDDEN);
    else lv_obj_clear_flag(stripe_, LV_OBJ_FLAG_HIDDEN);
    const lv_font_t* value_font = &race_digits_48;
    if (geometry.size == TileSize::Wide) value_font = &race_digits_64;
    else if (geometry.size == TileSize::Hero)
        value_font = &race_digits_96;
    else if (geometry.size == TileSize::GearHero)
        value_font = config.parameter == ParameterId::Gear ? &race_digits_140 : &race_digits_64;
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
        lv_obj_align(value_, LV_ALIGN_TOP_MID, 0, gear ? 100 : geometry.height > 200 ? 60 : 10);
        lv_obj_align(title_, LV_ALIGN_TOP_MID, 0, gear ? 26 : 2);
        if (!gear && (config.parameter == ParameterId::Rpm || config.parameter == ParameterId::Speed))
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
    display_filter_.reset();
    refresh_initialized_ = false;
    value_text_initialized_ = false;
    unit_text_initialized_ = false;
    raw_valid_initialized_ = false;
    warning_initialized_ = false;
    temperature_bar_initialized_ = false;
}
void TileView::hide() { if (root_) lv_obj_add_flag(root_, LV_OBJ_FLAG_HIDDEN); }
void TileView::arrangeUnit(const char* text, bool is_flag) {
    const bool row = size_ == TileSize::CompactRow;
    lv_obj_set_width(unit_, LV_SIZE_CONTENT);
    if (std::strcmp(text, "UNAVAILABLE") == 0) {
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
                      const VehicleState& state, bool supported,
                      bool warning_active,
                      uint32_t now_ms) {
    if (!root_ || lv_obj_has_flag(root_, LV_OBJ_FLAG_HIDDEN)) return;
    if (!warning_initialized_ || warning_active != last_warning_active_) {
          lv_obj_set_style_border_width(root_, warning_active ? 3 : base_border_ ? 1 : 0, 0);
        lv_obj_set_style_border_color(
            root_, warning_active ? UiTheme::red() : UiTheme::border(), 0);
        warning_initialized_ = true;
        last_warning_active_ = warning_active;
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
        lv_obj_set_style_bg_color(
            root_, lv_color_hex(presentation.background_rgb), 0);
        lv_obj_set_style_bg_color(
            stripe_, UiTheme::blue(), 0);
        if (presentation.state == FlagTileState::Unavailable) {
            lv_obj_set_style_bg_opa(value_, LV_OPA_TRANSP, 0);
            lv_obj_set_style_pad_all(value_,0,0);
            lv_obj_set_style_text_color(value_, UiTheme::muted(), 0);
            unit_text = "UNAVAILABLE";
        } else {
            std::snprintf(value_text, sizeof(value_text), "%s",
                          presentation.status_text);
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
    } else if (!supported) {
        unit_text = "UNAVAILABLE";
    } else if (signal.valid) {
        const PresentedValue p = UnitPresenter::present(
            config.parameter, signal.value, units);
        char format[8]; char buffer[32];
        std::snprintf(format, sizeof(format), "%%.%uf", static_cast<unsigned>(config.decimals));
        std::snprintf(buffer, sizeof(buffer), format, static_cast<double>(p.value));
        std::snprintf(value_text, sizeof(value_text), "%s", buffer);
        unit_text = std::strcmp(p.unit,"lambda") == 0 ? "" : p.unit;
        if (size_ == TileSize::Hero && config.parameter == ParameterId::Rpm) unit_text = "RPM";
    }

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
    arrangeRow(is_flag);
}
TileAddress TileView::address() const { return address_; }
void TileView::arrangeRow(bool is_flag) {
    if(size_!=TileSize::CompactRow || is_flag)return;
    const int border=lv_obj_get_style_border_width(root_,0);
    const int center=geometry_.height/2-border;
    lv_obj_set_width(title_,120);
    lv_obj_set_style_text_align(title_,LV_TEXT_ALIGN_CENTER,0);
    lv_obj_set_style_text_color(title_,UiTheme::muted(),0);
    lv_obj_set_pos(title_,12-border,opticalTextY(lv_label_get_text(title_),&lv_font_montserrat_16,center));
    const auto* font=lv_obj_get_style_text_font(value_,0);
    lv_point_t v,u;
    lv_txt_get_size(&v,lv_label_get_text(value_),font,0,0,LV_COORD_MAX,LV_TEXT_FLAG_NONE);
    const bool unavailable=std::strcmp(lv_label_get_text(unit_),"UNAVAILABLE")==0;
    lv_txt_get_size(&u,lv_label_get_text(unit_),&lv_font_montserrat_16,0,0,LV_COORD_MAX,LV_TEXT_FLAG_NONE);
    const int gap=u.x && !unavailable ? 8:0;
    const int group_width=v.x+gap+(unavailable ? 0:u.x);
    const int x=228-border-group_width/2;
    lv_obj_set_width(value_,v.x+1);
    lv_obj_set_style_text_align(value_,LV_TEXT_ALIGN_LEFT,0);
    lv_obj_set_pos(value_,x,opticalTextY(lv_label_get_text(value_),font,center-(unavailable ? 7:0)));
    if(!unavailable) {
        lv_obj_set_width(unit_,u.x+1);
        lv_obj_set_pos(unit_,x+v.x+gap,opticalTextY(lv_label_get_text(unit_),&lv_font_montserrat_16,center));
    }
}
