#include "ui.h"
#include <cstdio>
#include "ui/tile_engine.h"
#include "ui/ui_theme.h"
Ui* Ui::instance_ = nullptr;
namespace {
lv_obj_t* makeLabel(lv_obj_t* parent, const char* text, int x, int y,
                    const lv_font_t* font, lv_color_t color) {
    lv_obj_t* object = lv_label_create(parent);
    lv_label_set_text(object, text); lv_obj_set_pos(object, x, y);
    lv_obj_set_style_text_font(object, font, 0);
    lv_obj_set_style_text_color(object, color, 0); return object;
}
const char* canStatusText(CanStatus status) {
    switch (status) {
        case CanStatus::Disabled: return "DISABLED";
        case CanStatus::Waiting: return "WAITING";
        case CanStatus::Online: return "ONLINE";
        case CanStatus::Offline: return "OFFLINE";
        case CanStatus::InitFailed: return "INIT FAILED";
    }
    return "UNKNOWN";
}
}  // namespace
void Ui::styleScreen(lv_obj_t* screen) {
    lv_obj_set_style_bg_color(screen, UiTheme::background(), 0);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, 0);
    lv_obj_set_style_text_color(screen, UiTheme::text(), 0);
    lv_obj_clear_flag(screen, LV_OBJ_FLAG_SCROLLABLE);
}
void Ui::begin(const AppConfig& config) {
    instance_ = this; createDataPage(Page::Dash, config);
    createDataPage(Page::Track, config); createSettings(); load(Page::Dash);
}
void Ui::createDataPage(Page page, const AppConfig& config) {
    lv_obj_t*& screen = page == Page::Dash ? dash_ : track_;
    screen = lv_obj_create(nullptr); styleScreen(screen);
    (page == Page::Dash ? dash_shift_ : track_shift_).create(screen);
    if (page == Page::Dash) {
        for (std::size_t i = 0U; i < dash_tiles_.size(); ++i)
            dash_tiles_[i].create(screen, {PageId::Dash, static_cast<uint8_t>(i)}, tileEvent);
    } else {
        for (std::size_t i = 0U; i < track_tiles_.size(); ++i)
            track_tiles_[i].create(screen, {PageId::Track, static_cast<uint8_t>(i)}, tileEvent);
    }
    createNavigation(screen, page); applyLayout(page, config);
}
void Ui::createNavigation(lv_obj_t* parent, Page active) {
    constexpr const char* names[] = {"DASH", "TRACK", "SETTINGS"};
    constexpr int widths[] = {267, 266, 267}; int x = 0;
    for (int i = 0; i < 3; ++i) {
        lv_obj_t* button = lv_btn_create(parent);
        lv_obj_set_pos(button, x, TileLayout::kNavigationY);
        lv_obj_set_size(button, widths[i], TileLayout::kNavigationHeight); x += widths[i];
        lv_obj_set_style_radius(button, 0, 0); lv_obj_set_style_border_width(button, 0, 0);
        lv_obj_set_style_bg_color(button, static_cast<int>(active) == i
            ? lv_color_hex(0x153B57) : lv_color_hex(0x10151B), 0);
        lv_obj_add_event_cb(button, navEvent, LV_EVENT_CLICKED,
                            reinterpret_cast<void*>(static_cast<intptr_t>(i)));
        lv_obj_t* text = lv_label_create(button); lv_label_set_text(text, names[i]);
        lv_obj_set_style_text_font(text, &lv_font_montserrat_14, 0); lv_obj_center(text);
    }
}
void Ui::applyLayout(Page page, const AppConfig& config) {
    const PageId id = page == Page::Dash ? PageId::Dash : PageId::Track;
    const TilePlacementList placements = TileEngine::placements(id, config);
    if (page == Page::Dash) {
        for (TileView& tile : dash_tiles_) tile.hide();
        for (std::size_t i = 0U; i < placements.count; ++i) {
            const TilePlacement& p = placements.items[i];
            dash_tiles_[p.address.slot].apply(config.dash_tiles[p.address.slot], p.geometry);
        }
    } else {
        for (TileView& tile : track_tiles_) tile.hide();
        for (std::size_t i = 0U; i < placements.count; ++i) {
            const TilePlacement& p = placements.items[i];
            track_tiles_[p.address.slot].apply(config.track_tiles[p.address.slot], p.geometry);
        }
    }
}
void Ui::createSettings() {
    settings_ = lv_obj_create(nullptr); styleScreen(settings_);
    makeLabel(settings_, "SETTINGS", 24, 20, &lv_font_montserrat_28, UiTheme::text());
    makeLabel(settings_, "Display  •  Source & CAN  •  Shift lights  •  Units  •  Layouts",
              24, 58, &lv_font_montserrat_14, UiTheme::muted());
    lv_obj_t* panel = lv_obj_create(settings_); lv_obj_set_pos(panel, 24, 94);
    lv_obj_set_size(panel, 752, 310); lv_obj_set_style_bg_color(panel, UiTheme::panel(), 0);
    lv_obj_set_style_border_color(panel, UiTheme::border(), 0);
    settings_status_ = makeLabel(panel, "---", 20, 16, &lv_font_montserrat_20, UiTheme::text());
    makeLabel(panel, "Tap a dashboard tile to configure it.", 20, 252,
              &lv_font_montserrat_14, UiTheme::muted());
    createNavigation(settings_, Page::Settings);
}
void Ui::update(const VehicleState& state, const RuntimeDiagnostics&,
                const UiRuntimeStatus& status, const AppConfig& config,
                const TileWarningEngine& warnings) {
    applyLayout(Page::Dash, config); applyLayout(Page::Track, config);
    for (std::size_t i = 0U; i < dash_tiles_.size(); ++i)
        dash_tiles_[i].update(config.dash_tiles[i], config.units, state,
            warnings.isHighlighted({PageId::Dash, static_cast<uint8_t>(i)}));
    for (std::size_t i = 0U; i < track_tiles_.size(); ++i)
        track_tiles_[i].update(config.track_tiles[i], config.units, state,
            warnings.isHighlighted({PageId::Track, static_cast<uint8_t>(i)}));
    const SignalValue& rpm = state.get(ParameterId::Rpm);
    const uint16_t rpm_value = rpm.valid && rpm.value > 0.0f ? static_cast<uint16_t>(rpm.value) : 0U;
    dash_shift_.update(rpm_value, config.shift); track_shift_.update(rpm_value, config.shift);
    char buffer[384];
    std::snprintf(buffer, sizeof(buffer),
        "Brightness: %u%%\nSource: %s\nCAN: %s  •  %u kbit/s  •  timeout %u ms\nProfile: %s\nShift: %u / red %u / max %u rpm\nRX: %u  •  rejected: %u  •  mappings: %u",
        static_cast<unsigned>(config.brightness_percent),
        config.data_source == DataSource::Can ? "CAN" : "DEMO", canStatusText(status.can_status),
        static_cast<unsigned>(config.can.bitrate / 1000U), static_cast<unsigned>(config.can.timeout_ms),
        config.can.profile_id.data(), static_cast<unsigned>(config.shift.start_rpm),
        static_cast<unsigned>(config.shift.red_rpm), static_cast<unsigned>(config.shift.max_rpm),
        static_cast<unsigned>(status.received_frames), static_cast<unsigned>(status.rejected_frames),
        static_cast<unsigned>(status.decoder_mappings));
    lv_label_set_text(settings_status_, buffer);
}
void Ui::navEvent(lv_event_t* event) {
    if (!instance_) return;
    instance_->load(static_cast<Page>(reinterpret_cast<intptr_t>(lv_event_get_user_data(event))));
}
void Ui::tileEvent(lv_event_t*) {}
void Ui::load(Page page) {
    current_page_ = page;
    if (page == Page::Dash) lv_scr_load(dash_);
    if (page == Page::Track) lv_scr_load(track_);
    if (page == Page::Settings) lv_scr_load(settings_);
}
