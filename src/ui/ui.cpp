#include "ui.h"
#include <cstdio>
#include <cstring>
#include "ecu/can_profile_registry.h"
#include "ui/tile_engine.h"
#include "ui/parameter_options.h"
#include "ui/ui_theme.h"
#include "telemetry/parameter_registry.h"
#include "ui/unit_presenter.h"
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
lv_obj_t* makeButton(lv_obj_t* parent, const char* text, int x, int y,
                     int width, int height, lv_event_cb_t callback,
                     void* user_data = nullptr) {
    lv_obj_t* button = lv_btn_create(parent);
    lv_obj_set_pos(button, x, y); lv_obj_set_size(button, width, height);
    lv_obj_add_event_cb(button, callback, LV_EVENT_CLICKED, user_data);
    lv_obj_t* caption = lv_label_create(button); lv_label_set_text(caption, text);
    lv_obj_center(caption); return button;
}
lv_obj_t* makeSpinbox(lv_obj_t* parent, int x, int y, int width,
                      int32_t minimum, int32_t maximum, int32_t value,
                      uint8_t digits, uint8_t separator = 0U) {
    lv_obj_t* box = lv_spinbox_create(parent);
    lv_obj_set_pos(box, x, y); lv_obj_set_size(box, width, 38);
    lv_spinbox_set_range(box, minimum, maximum);
    lv_spinbox_set_digit_format(box, digits, separator);
    lv_spinbox_set_value(box, value); return box;
}
void makeStepper(lv_obj_t* parent, lv_obj_t* spinbox, int x, int y,
                 int button_width = 60) {
    makeButton(parent, "-", x, y, button_width, 34,
               Ui::spinDecreaseEvent, spinbox);
    makeButton(parent, "+", x + button_width + 6, y, button_width, 34,
               Ui::spinIncreaseEvent, spinbox);
}
enum SettingsAction : intptr_t {
    BrightnessPreview = 1,
    BrightnessCommit,
    SourceChanged,
    ProfileChanged,
    BitrateChanged,
    TimeoutDecrease,
    TimeoutIncrease,
    ShiftStartChanged,
    ShiftRedChanged,
    ShiftFlashChanged,
    ShiftMaxChanged,
    ShiftFlashEnabledChanged,
    UnitsChanged,
};

lv_obj_t* makeSettingsCard(lv_obj_t* parent, const char* title,
                           int x, int y, int width, int height,
                           lv_event_cb_t callback, void* user_data) {
    lv_obj_t* button = makeButton(parent, title, x, y, width, height,
                                  callback, user_data);
    lv_obj_set_style_bg_color(button, UiTheme::panel(), 0);
    lv_obj_set_style_border_color(button, UiTheme::border(), 0);
    lv_obj_set_style_border_width(button, 1, 0);
    lv_obj_set_style_radius(button, 10, 0);
    return button;
}
}  // namespace
void Ui::styleScreen(lv_obj_t* screen) {
    lv_obj_set_style_bg_color(screen, UiTheme::background(), 0);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, 0);
    lv_obj_set_style_text_color(screen, UiTheme::text(), 0);
    lv_obj_clear_flag(screen, LV_OBJ_FLAG_SCROLLABLE);
}
void Ui::begin(AppConfig& config, BoardDisplay& board) {
    instance_ = this; config_ = &config; board_ = &board;
    createDataPage(Page::Dash, config); createDataPage(Page::Track, config);
    board.setSoftwareBrightness(config.brightness_percent);
    update_policy_.takeLayoutDirty();
    load(Page::Dash);
}

void Ui::setDataContext(DataSource source, const CanProfile* profile) {
    capabilities_ = ParameterCapabilities::forSource(source, profile);
    update_policy_.markLayoutDirty();
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
void Ui::clearSettingsWidgets() {
    settings_status_ = nullptr;
    settings_message_ = nullptr;
    brightness_slider_ = nullptr;
    brightness_value_ = nullptr;
    source_dropdown_ = nullptr;
    profile_dropdown_ = nullptr;
    profile_recommendation_ = nullptr;
    bitrate_dropdown_ = nullptr;
    can_timeout_ = nullptr;
    shift_start_ = nullptr;
    shift_red_ = nullptr;
    shift_flash_ = nullptr;
    shift_max_ = nullptr;
    shift_start_value_ = nullptr;
    shift_red_value_ = nullptr;
    shift_flash_value_ = nullptr;
    shift_max_value_ = nullptr;
    shift_flash_enabled_ = nullptr;
    temp_unit_ = nullptr;
    pressure_unit_ = nullptr;
    speed_unit_ = nullptr;
    mixture_unit_ = nullptr;
    layout_labels_.fill(nullptr);
    layout_slots_.fill(0U);
}

lv_obj_t* Ui::createSettingsPanel(const char* title) {
    makeLabel(settings_, title, 310, 20, &lv_font_montserrat_24,
              UiTheme::text());
    settings_message_ = makeLabel(settings_, settings_feedback_, 660, 27,
                                  &lv_font_montserrat_12,
                                  UiTheme::yellow());
    lv_obj_t* panel = lv_obj_create(settings_);
    lv_obj_set_pos(panel, 16, 66);
    lv_obj_set_size(panel, 768, 348);
    lv_obj_set_style_bg_color(panel, UiTheme::background(), 0);
    lv_obj_set_style_border_width(panel, 0, 0);
    lv_obj_set_style_pad_all(panel, 0, 0);
    lv_obj_clear_flag(panel, LV_OBJ_FLAG_SCROLLABLE);
    return panel;
}

void Ui::showSettings(SettingsCategory category) {
    settings_flow_.open(category);
    clearSettingsWidgets();
    lv_obj_t* previous = settings_;
    settings_ = lv_obj_create(nullptr);
    styleScreen(settings_);
    lv_obj_t* panel = createSettingsPanel(
        category == SettingsCategory::Home ? "SETTINGS" :
        category == SettingsCategory::Display ? "DISPLAY" :
        category == SettingsCategory::DataCan ? "DATA & CAN" :
        category == SettingsCategory::ShiftLight ? "SHIFT LIGHT" :
        category == SettingsCategory::Units ? "UNITS" :
        category == SettingsCategory::Layouts ? "LAYOUTS" : "SYSTEM");
    if (category != SettingsCategory::Home) {
        makeButton(settings_, "< BACK", 20, 18, 120, 38,
                   settingsBackEvent);
    }
    if (category == SettingsCategory::Home) createSettingsHome(panel);
    if (category == SettingsCategory::Display) createDisplaySettings(panel);
    if (category == SettingsCategory::DataCan) createDataCanSettings(panel);
    if (category == SettingsCategory::ShiftLight) createShiftSettings(panel);
    if (category == SettingsCategory::Units) createUnitSettings(panel);
    if (category == SettingsCategory::Layouts) createLayoutSettings(panel);
    if (category == SettingsCategory::System) createSystemSettings(panel);
    createNavigation(settings_, Page::Settings);
    lv_scr_load(settings_);
    if (previous && previous != settings_) lv_obj_del_async(previous);
    update_policy_.setInteractionActive(false);
}

void Ui::createSettingsHome(lv_obj_t* panel) {
    constexpr const char* names[] = {
        "DISPLAY", "DATA & CAN", "SHIFT LIGHT",
        "UNITS", "LAYOUTS", "SYSTEM"};
    constexpr SettingsCategory categories[] = {
        SettingsCategory::Display, SettingsCategory::DataCan,
        SettingsCategory::ShiftLight, SettingsCategory::Units,
        SettingsCategory::Layouts, SettingsCategory::System};
    for (int index = 0; index < 6; ++index) {
        const int column = index % 3;
        const int row = index / 3;
        makeSettingsCard(panel, names[index], column * 256 + 6,
                         row * 166 + 6, 244, 154,
                         settingsCategoryEvent,
                         reinterpret_cast<void*>(static_cast<intptr_t>(
                             categories[index])));
    }
}

void Ui::createDisplaySettings(lv_obj_t* panel) {
    makeLabel(panel, "Brightness", 28, 40, &lv_font_montserrat_14,
              UiTheme::text());
    brightness_slider_ = lv_slider_create(panel);
    lv_obj_set_pos(brightness_slider_, 190, 50);
    lv_obj_set_size(brightness_slider_, 480, 20);
    lv_slider_set_range(brightness_slider_, 20, 100);
    lv_slider_set_value(brightness_slider_, config_->brightness_percent,
                        LV_ANIM_OFF);
    brightness_value_ = makeLabel(panel, "", 685, 42,
                                  &lv_font_montserrat_14, UiTheme::blue());
    char value[12];
    std::snprintf(value, sizeof(value), "%u%%",
                  static_cast<unsigned>(config_->brightness_percent));
    lv_label_set_text(brightness_value_, value);
    lv_obj_add_event_cb(brightness_slider_, settingsEvent,
                        LV_EVENT_VALUE_CHANGED,
                        reinterpret_cast<void*>(BrightnessPreview));
    lv_obj_add_event_cb(brightness_slider_, settingsEvent, LV_EVENT_RELEASED,
                        reinterpret_cast<void*>(BrightnessCommit));
    lv_obj_add_event_cb(brightness_slider_, settingsEvent, LV_EVENT_PRESS_LOST,
                        reinterpret_cast<void*>(BrightnessCommit));
    lv_obj_add_event_cb(brightness_slider_, settingsEvent, LV_EVENT_PRESSED,
                        reinterpret_cast<void*>(BrightnessPreview));
    settings_status_ = makeLabel(panel, "", 28, 125,
                                 &lv_font_montserrat_14, UiTheme::muted());
}

void Ui::createDataCanSettings(lv_obj_t* panel) {
    makeLabel(panel, "SOURCE", 24, 26, &lv_font_montserrat_12,
              UiTheme::muted());
    source_dropdown_ = lv_dropdown_create(panel);
    lv_obj_set_pos(source_dropdown_, 24, 48);
    lv_obj_set_size(source_dropdown_, 160, 44);
    lv_dropdown_set_options(source_dropdown_, "DEMO\nCAN");
    lv_dropdown_set_selected(source_dropdown_,
                             config_->data_source == DataSource::Can ? 1 : 0);
    lv_obj_add_event_cb(source_dropdown_, settingsEvent,
                        LV_EVENT_VALUE_CHANGED,
                        reinterpret_cast<void*>(SourceChanged));

    makeLabel(panel, "PROFILE", 208, 26, &lv_font_montserrat_12,
              UiTheme::muted());
    profile_dropdown_ = lv_dropdown_create(panel);
    lv_obj_set_pos(profile_dropdown_, 208, 48);
    lv_obj_set_size(profile_dropdown_, 160, 44);
    char profile_options[512] = "none";
    uint16_t selected_profile = 0U;
    std::size_t used = std::strlen(profile_options);
    for (std::size_t index = 0U; index < CanProfileRegistry::count(); ++index) {
        const CanProfile* profile = CanProfileRegistry::at(index);
        if (profile == nullptr) continue;
        const int written = std::snprintf(
            profile_options + used, sizeof(profile_options) - used,
            "\n%s", profile->name);
        if (written < 0 || static_cast<std::size_t>(written) >=
                               sizeof(profile_options) - used) {
            break;
        }
        used += static_cast<std::size_t>(written);
        if (std::strcmp(config_->can.profile_id.data(), profile->id) == 0) {
            selected_profile = static_cast<uint16_t>(index + 1U);
        }
    }
    lv_dropdown_set_options(profile_dropdown_, profile_options);
    lv_dropdown_set_selected(profile_dropdown_, selected_profile);
    lv_obj_add_event_cb(profile_dropdown_, settingsEvent,
                        LV_EVENT_VALUE_CHANGED,
                        reinterpret_cast<void*>(ProfileChanged));

    const CanProfile* selected = selected_profile == 0U
        ? nullptr : CanProfileRegistry::at(selected_profile - 1U);
    char recommendation[48] = "Recommended: select a profile";
    if (selected != nullptr) {
        std::snprintf(recommendation, sizeof(recommendation),
                      "Recommended: %u kbit/s",
                      static_cast<unsigned>(selected->default_bitrate / 1000U));
    }
    profile_recommendation_ = makeLabel(
        panel, recommendation, 208, 100,
        &lv_font_montserrat_12, UiTheme::muted());

    makeLabel(panel, "BITRATE", 392, 26, &lv_font_montserrat_12,
              UiTheme::muted());
    bitrate_dropdown_ = lv_dropdown_create(panel);
    lv_obj_set_pos(bitrate_dropdown_, 392, 48);
    lv_obj_set_size(bitrate_dropdown_, 180, 44);
    lv_dropdown_set_options(bitrate_dropdown_,
                            "125 kbit/s\n250 kbit/s\n500 kbit/s\n1000 kbit/s");
    const uint16_t bitrate_index = config_->can.bitrate == 125000U ? 0U :
        (config_->can.bitrate == 250000U ? 1U :
        (config_->can.bitrate == 500000U ? 2U : 3U));
    lv_dropdown_set_selected(bitrate_dropdown_, bitrate_index);
    lv_obj_add_event_cb(bitrate_dropdown_, settingsEvent,
                        LV_EVENT_VALUE_CHANGED,
                        reinterpret_cast<void*>(BitrateChanged));

    makeLabel(panel, "TIMEOUT MS", 598, 26, &lv_font_montserrat_12,
              UiTheme::muted());
    can_timeout_ = makeSpinbox(panel, 598, 48, 140, 100, 5000,
                               config_->can.timeout_ms, 4);
    lv_spinbox_set_step(can_timeout_, 100);
    makeButton(panel, "-", 598, 98, 64, 38, settingsEvent,
               reinterpret_cast<void*>(TimeoutDecrease));
    makeButton(panel, "+", 674, 98, 64, 38, settingsEvent,
               reinterpret_cast<void*>(TimeoutIncrease));
    settings_status_ = makeLabel(panel, "", 24, 174,
                                 &lv_font_montserrat_14, UiTheme::muted());
}

void Ui::createShiftSettings(lv_obj_t* panel) {
    constexpr const char* labels[] = {
        "START RPM", "RED RPM", "FLASH RPM", "MAX RPM"};
    lv_obj_t** sliders[] = {
        &shift_start_, &shift_red_, &shift_flash_, &shift_max_};
    lv_obj_t** value_labels[] = {
        &shift_start_value_, &shift_red_value_,
        &shift_flash_value_, &shift_max_value_};
    const int32_t values[] = {config_->shift.start_rpm,
                              config_->shift.red_rpm,
                              config_->shift.flash_rpm,
                              config_->shift.max_rpm};
    const intptr_t actions[] = {ShiftStartChanged, ShiftRedChanged,
                                ShiftFlashChanged, ShiftMaxChanged};
    for (int index = 0; index < 4; ++index) {
        const int y = 12 + index * 64;
        makeLabel(panel, labels[index], 24, y + 5, &lv_font_montserrat_14,
                  UiTheme::muted());
        *sliders[index] = lv_slider_create(panel);
        lv_obj_set_pos(*sliders[index], 160, y + 10);
        lv_obj_set_size(*sliders[index], 440, 18);
        lv_slider_set_range(*sliders[index], 0, 10000);
        lv_slider_set_value(*sliders[index], values[index], LV_ANIM_OFF);
        lv_obj_add_event_cb(*sliders[index], settingsEvent,
                            LV_EVENT_VALUE_CHANGED,
                            reinterpret_cast<void*>(actions[index]));
        *value_labels[index] = makeLabel(
            panel, "", 624, y + 4, &lv_font_montserrat_14, UiTheme::text());
    }
    makeLabel(panel, "FLASH ENABLED", 24, 282, &lv_font_montserrat_14,
              UiTheme::muted());
    shift_flash_enabled_ = lv_switch_create(panel);
    lv_obj_set_pos(shift_flash_enabled_, 174, 278);
    if (config_->shift.flash_enabled)
        lv_obj_add_state(shift_flash_enabled_, LV_STATE_CHECKED);
    lv_obj_add_event_cb(shift_flash_enabled_, settingsEvent,
                        LV_EVENT_VALUE_CHANGED,
                        reinterpret_cast<void*>(ShiftFlashEnabledChanged));
    makeLabel(panel, "Saved when leaving this screen", 420, 284,
              &lv_font_montserrat_12, UiTheme::muted());
    refreshShiftControls();
}

void Ui::refreshShiftControls() {
    if (!config_) return;
    lv_obj_t* sliders[] = {
        shift_start_, shift_red_, shift_flash_, shift_max_};
    lv_obj_t* labels[] = {
        shift_start_value_, shift_red_value_,
        shift_flash_value_, shift_max_value_};
    const uint16_t values[] = {
        config_->shift.start_rpm, config_->shift.red_rpm,
        config_->shift.flash_rpm, config_->shift.max_rpm};
    for (int index = 0; index < 4; ++index) {
        if (sliders[index])
            lv_slider_set_value(sliders[index], values[index], LV_ANIM_OFF);
        if (labels[index]) {
            char text[16];
            std::snprintf(text, sizeof(text), "%u RPM",
                          static_cast<unsigned>(values[index]));
            lv_label_set_text(labels[index], text);
        }
    }
}

void Ui::createUnitSettings(lv_obj_t* panel) {
    constexpr const char* labels[] = {"TEMPERATURE", "PRESSURE",
                                      "SPEED", "MIXTURE"};
    constexpr const char* options[] = {"Celsius\nFahrenheit", "bar\nkPa\npsi",
                                       "km/h\nmph", "lambda\nAFR"};
    lv_obj_t** dropdowns[] = {&temp_unit_, &pressure_unit_,
                              &speed_unit_, &mixture_unit_};
    const uint16_t selected[] = {
        static_cast<uint16_t>(config_->units.temperature),
        static_cast<uint16_t>(config_->units.pressure),
        static_cast<uint16_t>(config_->units.speed),
        static_cast<uint16_t>(config_->units.mixture)};
    for (int index = 0; index < 4; ++index) {
        const int x = 18 + index * 188;
        makeLabel(panel, labels[index], x, 56, &lv_font_montserrat_12,
                  UiTheme::muted());
        *dropdowns[index] = lv_dropdown_create(panel);
        lv_obj_set_pos(*dropdowns[index], x, 84);
        lv_obj_set_size(*dropdowns[index], 170, 48);
        lv_dropdown_set_options(*dropdowns[index], options[index]);
        lv_dropdown_set_selected(*dropdowns[index], selected[index]);
        lv_obj_add_event_cb(*dropdowns[index], settingsEvent,
                            LV_EVENT_VALUE_CHANGED,
                            reinterpret_cast<void*>(UnitsChanged));
    }
    makeLabel(panel, "Unit changes apply to DASH, TRACK and warnings", 170, 210,
              &lv_font_montserrat_14, UiTheme::muted());
}

void Ui::createLayoutSettings(lv_obj_t* panel) {
    makeButton(panel, "DASH", 20, 8, 170, 38, layoutSelectEvent,
               reinterpret_cast<void*>(static_cast<intptr_t>(PageId::Dash)));
    makeButton(panel, "TRACK", 202, 8, 170, 38, layoutSelectEvent,
               reinterpret_cast<void*>(static_cast<intptr_t>(PageId::Track)));
    const std::size_t count = settings_flow_.layout() == PageId::Dash
                                  ? AppConfig::kDashTileCount
                                  : AppConfig::kTrackTileCount;
    const std::size_t first = settings_flow_.firstSlot();
    for (std::size_t index = 0U;
         index < SettingsFlowModel::kSlotsPerPage && first + index < count;
         ++index) {
        const std::size_t slot = first + index;
        const TileConfig& tile = settings_flow_.layout() == PageId::Dash
                                     ? config_->dash_tiles[slot]
                                     : config_->track_tiles[slot];
        char text[80];
        std::snprintf(text, sizeof(text), "SLOT %u  %s  %s",
                      static_cast<unsigned>(slot + 1U),
                      parameterDescriptor(tile.parameter).name,
                      tile.visible ? "VISIBLE" : "HIDDEN");
        const int column = static_cast<int>(index % 2U);
        const int row = static_cast<int>(index / 2U);
        lv_obj_t* button = makeSettingsCard(
            panel, text, 12 + column * 376, 54 + row * 72, 364, 62,
            layoutSlotEvent, reinterpret_cast<void*>(slot));
        layout_labels_[index] = lv_obj_get_child(button, 0);
        layout_slots_[index] = slot;
    }
    lv_obj_t* previous = makeButton(
        panel, "< PREVIOUS", 12, 286, 160, 44, layoutPageEvent,
        reinterpret_cast<void*>(-1));
    if (settings_flow_.pageIndex() == 0U)
        lv_obj_add_state(previous, LV_STATE_DISABLED);
    char page[24];
    std::snprintf(page, sizeof(page), "%u / %u",
                  static_cast<unsigned>(settings_flow_.pageIndex() + 1U),
                  static_cast<unsigned>(settings_flow_.pageCount()));
    makeLabel(panel, page, 360, 298, &lv_font_montserrat_14,
              UiTheme::text());
    lv_obj_t* next = makeButton(
        panel, "NEXT >", 596, 286, 160, 44, layoutPageEvent,
        reinterpret_cast<void*>(1));
    if (settings_flow_.pageIndex() + 1U >= settings_flow_.pageCount())
        lv_obj_add_state(next, LV_STATE_DISABLED);
}

void Ui::createSystemSettings(lv_obj_t* panel) {
    makeLabel(panel, "Firmware and runtime information", 24, 26,
              &lv_font_montserrat_14, UiTheme::text());
    settings_status_ = makeLabel(panel, "", 24, 78,
                                 &lv_font_montserrat_14, UiTheme::muted());
    makeButton(panel, "RESET DASH", 24, 248, 220, 58, settingsResetEvent,
               reinterpret_cast<void*>(static_cast<intptr_t>(
                   SettingsResetTarget::DashLayout)));
    makeButton(panel, "RESET TRACK", 274, 248, 220, 58, settingsResetEvent,
               reinterpret_cast<void*>(static_cast<intptr_t>(
                   SettingsResetTarget::TrackLayout)));
    makeButton(panel, "FACTORY RESET", 524, 248, 220, 58,
               settingsResetEvent,
               reinterpret_cast<void*>(static_cast<intptr_t>(
                   SettingsResetTarget::Factory)));
}

void Ui::update(const VehicleState& state, const RuntimeDiagnostics& diagnostics,
                const UiRuntimeStatus& status, const AppConfig& config,
                TileWarningEngine& warnings) {
    if (update_policy_.allowLayoutUpdates() &&
        update_policy_.takeLayoutDirty()) {
        applyLayout(Page::Dash, config);
        applyLayout(Page::Track, config);
    }
    if (update_policy_.shouldUpdateData(PageId::Dash)) {
        for (std::size_t i = 0U; i < dash_tiles_.size(); ++i)
            dash_tiles_[i].update(config.dash_tiles[i], config.units, state,
                capabilities_.supports(config.dash_tiles[i].parameter),
                warnings.isHighlighted({PageId::Dash, static_cast<uint8_t>(i)}),
                diagnostics.uptime_ms);
    }
    if (update_policy_.shouldUpdateData(PageId::Track)) {
        for (std::size_t i = 0U; i < track_tiles_.size(); ++i)
            track_tiles_[i].update(config.track_tiles[i], config.units, state,
                capabilities_.supports(config.track_tiles[i].parameter),
                warnings.isHighlighted({PageId::Track, static_cast<uint8_t>(i)}),
                diagnostics.uptime_ms);
    }
    if (settings_status_ &&
        update_policy_.shouldUpdateSettingsStatus(diagnostics.uptime_ms)) {
        char buffer[256];
        if (settings_flow_.category() == SettingsCategory::Display) {
            std::snprintf(buffer, sizeof(buffer),
                "Free heap: %u KiB\nPSRAM total: %u KiB\nDisplay: 800 x 480 RGB565\nLVGL buffers: 250 KiB PSRAM",
                static_cast<unsigned>(diagnostics.free_heap / 1024U),
                static_cast<unsigned>(diagnostics.psram_total / 1024U));
        } else if (settings_flow_.category() == SettingsCategory::DataCan) {
            std::snprintf(buffer, sizeof(buffer),
                "CAN: %s  |  receive-only\nRX: %u  |  rejected: %u  |  mappings: %u",
                canStatusText(status.can_status),
                static_cast<unsigned>(status.received_frames),
                static_cast<unsigned>(status.rejected_frames),
                static_cast<unsigned>(status.decoder_mappings));
        } else {
            std::snprintf(buffer, sizeof(buffer),
                "Uptime: %u s\nFree heap: %u KiB\nPSRAM total: %u KiB\nUI updates: %u",
                static_cast<unsigned>(diagnostics.uptime_ms / 1000U),
                static_cast<unsigned>(diagnostics.free_heap / 1024U),
                static_cast<unsigned>(diagnostics.psram_total / 1024U),
                static_cast<unsigned>(diagnostics.ui_updates));
        }
        lv_label_set_text(settings_status_, buffer);
    }
    if (commit_toast_ &&
        static_cast<int32_t>(lv_tick_get() - commit_toast_until_ms_) >= 0) {
        lv_obj_add_flag(commit_toast_, LV_OBJ_FLAG_HIDDEN);
    }
    if (update_policy_.allowModalUpdates()) {
        updateWarningModal(warnings);
    }
}

void Ui::updateShiftLight(const VehicleState& state, uint32_t now_ms,
                          const ShiftLightConfig& config) {
    if (!update_policy_.allowShiftLightUpdates()) return;
    const SignalValue& rpm = state.get(ParameterId::Rpm);
    const uint16_t rpm_value = rpm.valid && rpm.value > 0.0f
                                   ? static_cast<uint16_t>(rpm.value)
                                   : 0U;
    if (update_policy_.shouldUpdateData(PageId::Dash)) {
        dash_shift_.update(rpm_value, rpm.valid, now_ms, config);
    }
    if (update_policy_.shouldUpdateData(PageId::Track)) {
        track_shift_.update(rpm_value, rpm.valid, now_ms, config);
    }
}
void Ui::navEvent(lv_event_t* event) {
    if (!instance_) return;
    const Page destination = static_cast<Page>(
        reinterpret_cast<intptr_t>(lv_event_get_user_data(event)));
    if (instance_->current_page_ == Page::Settings) {
        instance_->queueSettingsOnExit();
    }
    instance_->load(destination);
}
void Ui::tileEvent(lv_event_t* event) {
    if (!instance_) return;
    auto* tile = static_cast<TileView*>(lv_event_get_user_data(event));
    if (tile) instance_->openEditor(tile->address());
}
void Ui::load(Page page) {
    current_page_ = page;
    update_policy_.activate(page == Page::Dash ? PageId::Dash :
                            (page == Page::Track ? PageId::Track : PageId::Settings));
    if (page == Page::Dash) lv_scr_load(dash_);
    if (page == Page::Track) lv_scr_load(track_);
    if (page == Page::Settings) showSettings(SettingsCategory::Home);
}

bool Ui::takeConfigCommit(ConfigCommitRequest& request) {
    return commit_model_.take(request);
}

void Ui::completeConfigCommit(uint32_t revision, bool success) {
    commit_model_.complete(revision, success);
    showCommitFeedback(success ? "SAVED" : "SAVE ERROR");
    lv_obj_invalidate(lv_scr_act());
}

void Ui::openEditor(TileAddress address) {
    if (!config_) return;
    if (editor_screen_) closeEditor();
    editor_return_page_ = current_page_;
    editor_return_category_ = settings_flow_.category();
    if (!editor_.open(address, *config_)) return;
    const TileConfig& tile = editor_.draft().tile;
    editor_screen_ = lv_obj_create(nullptr);
    styleScreen(editor_screen_);
    makeLabel(editor_screen_, "TILE SETTINGS", 20, 10,
              &lv_font_montserrat_24, UiTheme::text());

    makeLabel(editor_screen_, "PARAMETER", 20, 48,
              &lv_font_montserrat_12, UiTheme::muted());
    editor_parameter_ = lv_dropdown_create(editor_screen_);
    lv_obj_set_pos(editor_parameter_, 20, 66);
    lv_obj_set_size(editor_parameter_, 360, 42);
    const CanProfile* profile = CanProfileRegistry::find(
        config_->can.profile_id.data());
    editor_parameter_options_ = ParameterOptions::build(
        config_->data_source, profile, tile.parameter);
    char parameter_options[2048]{};
    if (!editor_parameter_options_.write(parameter_options,
                                         sizeof(parameter_options))) {
        std::strncpy(parameter_options, "RPM", sizeof(parameter_options) - 1U);
    }
    lv_dropdown_set_options(editor_parameter_, parameter_options);
    uint16_t selected_parameter = 0U;
    for (std::size_t index = 0U;
         index < editor_parameter_options_.count(); ++index) {
        if (editor_parameter_options_.parameterAt(index) == tile.parameter) {
            selected_parameter = static_cast<uint16_t>(index);
            break;
        }
    }
    lv_dropdown_set_selected(editor_parameter_, selected_parameter);
    lv_obj_add_event_cb(editor_parameter_, editorEvent, LV_EVENT_VALUE_CHANGED,
                        reinterpret_cast<void*>(3));
    editor_visible_ = lv_checkbox_create(editor_screen_);
    lv_obj_set_pos(editor_visible_, 410, 74);
    lv_checkbox_set_text(editor_visible_, "Visible");
    if (tile.visible) lv_obj_add_state(editor_visible_, LV_STATE_CHECKED);
    editor_decimals_label_ = makeLabel(
        editor_screen_, "DECIMALS", 610, 48,
        &lv_font_montserrat_12, UiTheme::muted());
    editor_decimals_ = lv_dropdown_create(editor_screen_);
    lv_obj_set_pos(editor_decimals_, 610, 66);
    lv_obj_set_size(editor_decimals_, 150, 42);
    lv_dropdown_set_options(editor_decimals_, "0\n1\n2\n3");
    lv_dropdown_set_selected(editor_decimals_, tile.decimals);

    editor_numeric_panel_ = lv_obj_create(editor_screen_);
    lv_obj_set_pos(editor_numeric_panel_, 0, 112);
    lv_obj_set_size(editor_numeric_panel_, 800, 298);
    lv_obj_set_style_bg_opa(editor_numeric_panel_, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(editor_numeric_panel_, 0, 0);
    lv_obj_set_style_pad_all(editor_numeric_panel_, 0, 0);
    lv_obj_clear_flag(editor_numeric_panel_, LV_OBJ_FLAG_SCROLLABLE);

    editor_temperature_bar_ = lv_checkbox_create(editor_numeric_panel_);
    lv_obj_set_pos(editor_temperature_bar_, 20, 14);
    lv_checkbox_set_text(editor_temperature_bar_, "Temperature bar");
    makeLabel(editor_numeric_panel_, "MIN", 210, 8, &lv_font_montserrat_12, UiTheme::muted());
    editor_temperature_minimum_ = makeSpinbox(
        editor_numeric_panel_, 210, 26, 140, -9990, 9990, 0, 4, 1);
    lv_spinbox_set_step(editor_temperature_minimum_, 1);
    makeStepper(editor_numeric_panel_, editor_temperature_minimum_, 210, 68, 65);
    makeLabel(editor_numeric_panel_, "READY", 390, 8, &lv_font_montserrat_12, UiTheme::muted());
    editor_temperature_ready_ = makeSpinbox(
        editor_numeric_panel_, 390, 26, 140, -9990, 9990, 0, 4, 1);
    lv_spinbox_set_step(editor_temperature_ready_, 1);
    makeStepper(editor_numeric_panel_, editor_temperature_ready_, 390, 68, 65);
    makeLabel(editor_numeric_panel_, "MAX", 570, 8, &lv_font_montserrat_12, UiTheme::muted());
    editor_temperature_maximum_ = makeSpinbox(
        editor_numeric_panel_, 570, 26, 140, -9990, 9990, 0, 4, 1);
    lv_spinbox_set_step(editor_temperature_maximum_, 1);
    makeStepper(editor_numeric_panel_, editor_temperature_maximum_, 570, 68, 65);
    loadEditorTemperatureControls(tile.temperature_bar);

    editor_warning_ = lv_checkbox_create(editor_numeric_panel_); lv_obj_set_pos(editor_warning_, 20, 140);
    lv_checkbox_set_text(editor_warning_, "Enable WARNING");
    if (tile.warning.enabled) lv_obj_add_state(editor_warning_, LV_STATE_CHECKED);
    makeLabel(editor_numeric_panel_, "Direction", 190, 134, &lv_font_montserrat_12, UiTheme::muted());
    editor_direction_ = lv_dropdown_create(editor_numeric_panel_); lv_obj_set_pos(editor_direction_, 190, 152);
    lv_obj_set_size(editor_direction_, 120, 42); lv_dropdown_set_options(editor_direction_, "Above\nBelow");
    lv_dropdown_set_selected(editor_direction_, static_cast<uint16_t>(tile.warning.direction));
    makeLabel(editor_numeric_panel_, "Threshold", 330, 134, &lv_font_montserrat_12, UiTheme::muted());
    editor_threshold_ = makeSpinbox(editor_numeric_panel_, 330, 152, 130, 0, 9990,
        static_cast<int32_t>(tile.warning.threshold_native * 10.0f), 4, 1);
    lv_spinbox_set_step(editor_threshold_, 1); makeStepper(editor_numeric_panel_, editor_threshold_, 330, 194, 60);
    makeLabel(editor_numeric_panel_, "Hysteresis", 480, 134, &lv_font_montserrat_12, UiTheme::muted());
    editor_hysteresis_ = makeSpinbox(editor_numeric_panel_, 480, 152, 130, 0, 9990,
        static_cast<int32_t>(tile.warning.hysteresis_native * 10.0f), 4, 1);
    lv_spinbox_set_step(editor_hysteresis_, 1); makeStepper(editor_numeric_panel_, editor_hysteresis_, 480, 194, 60);
    makeLabel(editor_numeric_panel_, "Delay ms", 630, 134, &lv_font_montserrat_12, UiTheme::muted());
    editor_delay_ = makeSpinbox(editor_numeric_panel_, 630, 152, 120, 0, 10000,
        tile.warning.delay_ms, 5);
    lv_spinbox_set_step(editor_delay_, 100); makeStepper(editor_numeric_panel_, editor_delay_, 630, 194, 55);
    makeLabel(editor_numeric_panel_, "Temperature: MIN < READY < MAX. Alarm range: 0.0–999.0.", 20, 246,
              &lv_font_montserrat_12, UiTheme::muted());

    editor_flag_panel_ = lv_obj_create(editor_screen_);
    lv_obj_set_pos(editor_flag_panel_, 0, 112);
    lv_obj_set_size(editor_flag_panel_, 800, 298);
    lv_obj_set_style_bg_opa(editor_flag_panel_, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(editor_flag_panel_, 0, 0);
    lv_obj_clear_flag(editor_flag_panel_, LV_OBJ_FLAG_SCROLLABLE);
    makeLabel(editor_flag_panel_, "ACTIVE COLOR", 250, 60,
              &lv_font_montserrat_14, UiTheme::muted());
    editor_flag_color_ = lv_dropdown_create(editor_flag_panel_);
    lv_obj_set_pos(editor_flag_color_, 250, 88);
    lv_obj_set_size(editor_flag_color_, 300, 48);
    lv_dropdown_set_options(editor_flag_color_, "YELLOW\nGREEN\nRED");
    lv_dropdown_set_selected(editor_flag_color_,
                             static_cast<uint16_t>(tile.flag_active_color));
    makeLabel(editor_flag_panel_,
              "OFF stays neutral. ON uses the selected accent color.",
              190, 164, &lv_font_montserrat_14, UiTheme::text());

    makeButton(editor_screen_, "CANCEL", 20, 420, 180, 48, editorEvent,
               reinterpret_cast<void*>(1));
    makeButton(editor_screen_, "SAVE TILE", 590, 420, 190, 48, editorEvent,
               reinterpret_cast<void*>(2));
    editor_message_ = makeLabel(editor_screen_, "", 235, 436,
                                &lv_font_montserrat_14, UiTheme::red());
    refreshEditorParameterControls(tile.parameter, false);
    update_policy_.activate(UiActivity::TileEditor);
    lv_scr_load(editor_screen_);
}

void Ui::refreshEditorParameterControls(ParameterId parameter,
                                        bool load_temperature_defaults) {
    if (load_temperature_defaults) {
        loadEditorTemperatureControls(defaultTemperatureBarConfig(parameter));
    }
    const bool is_flag = parameterDescriptor(parameter).kind ==
                         ParameterKind::Flag;
    if (is_flag) {
        lv_obj_add_flag(editor_numeric_panel_, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(editor_flag_panel_, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(editor_decimals_label_, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(editor_decimals_, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_clear_flag(editor_numeric_panel_, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(editor_flag_panel_, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(editor_decimals_label_, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(editor_decimals_, LV_OBJ_FLAG_HIDDEN);
    }
}

void Ui::loadEditorTemperatureControls(const TemperatureBarConfig& config) {
    if (config.enabled) lv_obj_add_state(editor_temperature_bar_, LV_STATE_CHECKED);
    else lv_obj_clear_state(editor_temperature_bar_, LV_STATE_CHECKED);
    lv_spinbox_set_value(editor_temperature_minimum_,
                         static_cast<int32_t>(config.minimum_native * 10.0f));
    lv_spinbox_set_value(editor_temperature_ready_,
                         static_cast<int32_t>(config.ready_native * 10.0f));
    lv_spinbox_set_value(editor_temperature_maximum_,
                         static_cast<int32_t>(config.maximum_native * 10.0f));
}

void Ui::closeEditor() {
    if (!editor_screen_) {
        editor_.cancel();
        return;
    }
    lv_obj_t* previous = editor_screen_;
    editor_screen_ = nullptr;
    editor_.cancel();
    if (editor_return_page_ == Page::Settings) {
        current_page_ = Page::Settings;
        update_policy_.activate(PageId::Settings);
        showSettings(editor_return_category_);
    } else {
        load(editor_return_page_);
    }
    lv_obj_del_async(previous);
}

void Ui::saveEditor() {
    if (!config_ || !editor_.isOpen()) return;
    editor_.setParameter(editor_parameter_options_.parameterAt(
        lv_dropdown_get_selected(editor_parameter_)));
    editor_.setVisible(lv_obj_has_state(editor_visible_, LV_STATE_CHECKED));
    editor_.setDecimals(static_cast<uint8_t>(lv_dropdown_get_selected(editor_decimals_)));
    TileWarningConfig warning;
    warning.enabled = lv_obj_has_state(editor_warning_, LV_STATE_CHECKED);
    warning.direction = static_cast<WarningDirection>(lv_dropdown_get_selected(editor_direction_));
    warning.threshold_native = static_cast<float>(lv_spinbox_get_value(editor_threshold_)) / 10.0f;
    warning.hysteresis_native = static_cast<float>(lv_spinbox_get_value(editor_hysteresis_)) / 10.0f;
    warning.delay_ms = static_cast<uint16_t>(lv_spinbox_get_value(editor_delay_));
    editor_.setWarning(warning);
    TemperatureBarConfig temperature_bar;
    temperature_bar.enabled = lv_obj_has_state(
        editor_temperature_bar_, LV_STATE_CHECKED);
    temperature_bar.minimum_native = static_cast<float>(
        lv_spinbox_get_value(editor_temperature_minimum_)) / 10.0f;
    temperature_bar.ready_native = static_cast<float>(
        lv_spinbox_get_value(editor_temperature_ready_)) / 10.0f;
    temperature_bar.maximum_native = static_cast<float>(
        lv_spinbox_get_value(editor_temperature_maximum_)) / 10.0f;
    editor_.setTemperatureBar(temperature_bar);
    editor_.setFlagActiveColor(static_cast<FlagActiveColor>(
        lv_dropdown_get_selected(editor_flag_color_)));
    AppConfig candidate = *config_;
    if (!editor_.applyTo(candidate) || !stageSettings(candidate, false)) {
        lv_label_set_text(editor_message_, "SAVE FAILED"); return;
    }
    update_policy_.markLayoutDirty();
    const bool refresh_layout = current_page_ == Page::Settings &&
        settings_flow_.category() == SettingsCategory::Layouts;
    closeEditor();
    queueSettingsOnExit();
    if (refresh_layout) showSettings(SettingsCategory::Layouts);
}

void Ui::showSettingsMessage(const char* message) {
    if (settings_message_) lv_label_set_text(settings_message_, message);
}

void Ui::showCommitFeedback(const char* message) {
    settings_feedback_ = message;
    showSettingsMessage(message);
    if (!commit_toast_) {
        commit_toast_ = lv_label_create(lv_layer_top());
        lv_obj_set_pos(commit_toast_, 640, 10);
        lv_obj_set_width(commit_toast_, 140);
        lv_obj_set_style_text_align(commit_toast_, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_set_style_text_color(commit_toast_, UiTheme::text(), 0);
        lv_obj_set_style_bg_opa(commit_toast_, LV_OPA_COVER, 0);
        lv_obj_set_style_radius(commit_toast_, 6, 0);
        lv_obj_set_style_pad_all(commit_toast_, 8, 0);
        lv_obj_clear_flag(commit_toast_, LV_OBJ_FLAG_CLICKABLE);
    }
    lv_obj_set_style_bg_color(
        commit_toast_, std::strcmp(message, "SAVE ERROR") == 0
                           ? UiTheme::red()
                           : lv_color_hex(0x153B57),
        0);
    lv_label_set_text(commit_toast_, message);
    lv_obj_clear_flag(commit_toast_, LV_OBJ_FLAG_HIDDEN);
    lv_obj_move_foreground(commit_toast_);
    commit_toast_until_ms_ = lv_tick_get() + 1500U;
}

bool Ui::stageSettings(AppConfig candidate, bool reconfigure_runtime) {
    if (!config_ || !candidate.validate().valid) {
        showSettingsMessage("SAVE ERROR");
        if (board_ && config_)
            board_->setSoftwareBrightness(config_->brightness_percent);
        return false;
    }
    if (std::memcmp(&candidate, config_, sizeof(candidate)) == 0) {
        return true;
    }
    *config_ = candidate;
    commit_model_.markDirty(reconfigure_runtime);
    if (board_) board_->setSoftwareBrightness(config_->brightness_percent);
    settings_feedback_ = "UNSAVED";
    showSettingsMessage(settings_feedback_);
    return true;
}

void Ui::queueSettingsOnExit() {
    if (!config_ || !commit_model_.queueOnExit(*config_)) return;
    showCommitFeedback("SAVING");
}

void Ui::openResetConfirmation(SettingsResetTarget target) {
    closeResetConfirmation();
    settings_flow_.requestReset(target);
    reset_overlay_ = lv_obj_create(lv_layer_top());
    lv_obj_set_size(reset_overlay_, 600, 260);
    lv_obj_center(reset_overlay_);
    lv_obj_set_style_bg_color(reset_overlay_, UiTheme::panel(), 0);
    lv_obj_set_style_border_color(reset_overlay_, UiTheme::red(), 0);
    lv_obj_set_style_border_width(reset_overlay_, 3, 0);
    lv_obj_clear_flag(reset_overlay_, LV_OBJ_FLAG_SCROLLABLE);
    const char* label = target == SettingsResetTarget::DashLayout
                            ? "RESET DASH LAYOUT?"
                        : target == SettingsResetTarget::TrackLayout
                            ? "RESET TRACK LAYOUT?"
                            : "FACTORY RESET ALL SETTINGS?";
    lv_obj_t* title = makeLabel(reset_overlay_, label, 20, 34,
                                &lv_font_montserrat_24, UiTheme::text());
    lv_obj_set_width(title, 560);
    lv_obj_set_style_text_align(title, LV_TEXT_ALIGN_CENTER, 0);
    makeLabel(reset_overlay_, "This operation cannot be undone", 164, 98,
              &lv_font_montserrat_14, UiTheme::muted());
    makeButton(reset_overlay_, "CANCEL", 40, 166, 220, 56,
               settingsConfirmEvent, reinterpret_cast<void*>(0));
    makeButton(reset_overlay_, "CONFIRM", 340, 166, 220, 56,
               settingsConfirmEvent, reinterpret_cast<void*>(1));
}

void Ui::closeResetConfirmation() {
    if (reset_overlay_) {
        lv_obj_del_async(reset_overlay_);
        reset_overlay_ = nullptr;
    }
    settings_flow_.cancelReset();
}

void Ui::confirmReset() {
    if (!config_ || !settings_flow_.resetPending()) return;
    const SettingsResetTarget target = settings_flow_.pendingReset();
    bool staged = false;
    if (target == SettingsResetTarget::DashLayout) {
        AppConfig candidate = *config_;
        candidate.dash_tiles = AppConfig::defaults().dash_tiles;
        staged = stageSettings(candidate, false);
    } else if (target == SettingsResetTarget::TrackLayout) {
        AppConfig candidate = *config_;
        candidate.track_tiles = AppConfig::defaults().track_tiles;
        staged = stageSettings(candidate, false);
    } else {
        *config_ = AppConfig::defaults();
        commit_model_.queueFactoryReset();
        showCommitFeedback("SAVING");
        staged = true;
    }
    closeResetConfirmation();
    if (!staged) {
        showSettingsMessage("SAVE ERROR");
        return;
    }
    update_policy_.markLayoutDirty();
    if (target == SettingsResetTarget::Factory) {
        if (board_) board_->setSoftwareBrightness(config_->brightness_percent);
        showSettings(SettingsCategory::Home);
    } else {
        queueSettingsOnExit();
    }
}

void Ui::editorEvent(lv_event_t* event) {
    if (!instance_) return;
    const intptr_t action = reinterpret_cast<intptr_t>(lv_event_get_user_data(event));
    if (action == 1) instance_->closeEditor();
    if (action == 2) instance_->saveEditor();
    if (action == 3) {
        const ParameterId parameter =
            instance_->editor_parameter_options_.parameterAt(
                lv_dropdown_get_selected(instance_->editor_parameter_));
        instance_->refreshEditorParameterControls(parameter, true);
    }
}

void Ui::settingsEvent(lv_event_t* event) {
    if (!instance_ || !instance_->config_) return;
    const intptr_t action = reinterpret_cast<intptr_t>(lv_event_get_user_data(event));
    AppConfig candidate = *instance_->config_;

    if (action == BrightnessPreview) {
        instance_->update_policy_.setInteractionActive(true);
        const uint8_t value = static_cast<uint8_t>(
            lv_slider_get_value(instance_->brightness_slider_));
        if (instance_->board_) instance_->board_->setSoftwareBrightness(value);
        if (instance_->brightness_value_) {
            char text[12];
            std::snprintf(text, sizeof(text), "%u%%",
                          static_cast<unsigned>(value));
            lv_label_set_text(instance_->brightness_value_, text);
        }
        return;
    }
    if (action == BrightnessCommit) {
        instance_->update_policy_.setInteractionActive(false);
        candidate.brightness_percent = static_cast<uint8_t>(
            lv_slider_get_value(instance_->brightness_slider_));
        instance_->stageSettings(candidate, false);
        return;
    }
    if (action == SourceChanged) {
        candidate.data_source = lv_dropdown_get_selected(
            instance_->source_dropdown_) == 1U ? DataSource::Can
                                                : DataSource::Demo;
        instance_->stageSettings(candidate, true);
        return;
    }
    if (action == ProfileChanged) {
        const uint16_t selected = lv_dropdown_get_selected(
            instance_->profile_dropdown_);
        candidate.can.profile_id.fill('\0');
        const CanProfile* profile = selected == 0U
            ? nullptr : CanProfileRegistry::at(selected - 1U);
        const char* id = profile == nullptr ? "none" : profile->id;
        std::strncpy(candidate.can.profile_id.data(), id,
                     candidate.can.profile_id.size() - 1U);
        if (instance_->profile_recommendation_ != nullptr) {
            char recommendation[48] = "Recommended: select a profile";
            if (profile != nullptr) {
                std::snprintf(
                    recommendation, sizeof(recommendation),
                    "Recommended: %u kbit/s",
                    static_cast<unsigned>(profile->default_bitrate / 1000U));
            }
            lv_label_set_text(instance_->profile_recommendation_, recommendation);
        }
        instance_->stageSettings(candidate, true);
        return;
    }
    if (action == BitrateChanged) {
        constexpr uint32_t bitrates[] = {125000U, 250000U, 500000U, 1000000U};
        candidate.can.bitrate = bitrates[lv_dropdown_get_selected(
            instance_->bitrate_dropdown_)];
        instance_->stageSettings(candidate, true);
        return;
    }
    if (action == TimeoutDecrease || action == TimeoutIncrease) {
        if (action == TimeoutDecrease)
            lv_spinbox_decrement(instance_->can_timeout_);
        else
            lv_spinbox_increment(instance_->can_timeout_);
        candidate.can.timeout_ms = static_cast<uint32_t>(
            lv_spinbox_get_value(instance_->can_timeout_));
        instance_->stageSettings(candidate, true);
        return;
    }

    ShiftField field = ShiftField::Start;
    lv_obj_t* slider = nullptr;
    if (action == ShiftStartChanged) {
        field = ShiftField::Start; slider = instance_->shift_start_;
    } else if (action == ShiftRedChanged) {
        field = ShiftField::Red; slider = instance_->shift_red_;
    } else if (action == ShiftFlashChanged) {
        field = ShiftField::Flash; slider = instance_->shift_flash_;
    } else if (action == ShiftMaxChanged) {
        field = ShiftField::Maximum; slider = instance_->shift_max_;
    }
    if (slider) {
        candidate.shift = SettingsFlowModel::correctedShift(
            candidate.shift, field,
            static_cast<uint16_t>(lv_slider_get_value(slider)));
        instance_->stageSettings(candidate, false);
        instance_->refreshShiftControls();
        return;
    }
    if (action == ShiftFlashEnabledChanged) {
        candidate.shift.flash_enabled = lv_obj_has_state(
            instance_->shift_flash_enabled_, LV_STATE_CHECKED);
        instance_->stageSettings(candidate, false);
        return;
    }

    if (action == UnitsChanged) {
        candidate.units.temperature = static_cast<TemperatureUnit>(
            lv_dropdown_get_selected(instance_->temp_unit_));
        candidate.units.pressure = static_cast<PressureUnit>(
            lv_dropdown_get_selected(instance_->pressure_unit_));
        candidate.units.speed = static_cast<SpeedUnit>(
            lv_dropdown_get_selected(instance_->speed_unit_));
        candidate.units.mixture = static_cast<MixtureUnit>(
            lv_dropdown_get_selected(instance_->mixture_unit_));
        instance_->stageSettings(candidate, false);
    }
}

void Ui::settingsCategoryEvent(lv_event_t* event) {
    if (!instance_) return;
    instance_->showSettings(static_cast<SettingsCategory>(
        reinterpret_cast<intptr_t>(lv_event_get_user_data(event))));
}

void Ui::settingsResetEvent(lv_event_t* event) {
    if (!instance_) return;
    instance_->openResetConfirmation(static_cast<SettingsResetTarget>(
        reinterpret_cast<intptr_t>(lv_event_get_user_data(event))));
}

void Ui::settingsConfirmEvent(lv_event_t* event) {
    if (!instance_) return;
    const intptr_t confirm = reinterpret_cast<intptr_t>(
        lv_event_get_user_data(event));
    if (confirm != 0) instance_->confirmReset();
    else instance_->closeResetConfirmation();
}

void Ui::settingsBackEvent(lv_event_t*) {
    if (!instance_) return;
    instance_->queueSettingsOnExit();
    instance_->settings_flow_.backToHome();
    instance_->showSettings(SettingsCategory::Home);
}

void Ui::layoutPageEvent(lv_event_t* event) {
    if (!instance_) return;
    const intptr_t direction = reinterpret_cast<intptr_t>(
        lv_event_get_user_data(event));
    const bool changed = direction < 0 ? instance_->settings_flow_.previousPage()
                                       : instance_->settings_flow_.nextPage();
    if (changed) instance_->showSettings(SettingsCategory::Layouts);
}

void Ui::layoutSelectEvent(lv_event_t* event) {
    if (!instance_) return;
    instance_->settings_flow_.selectLayout(static_cast<PageId>(
        reinterpret_cast<intptr_t>(lv_event_get_user_data(event))));
    instance_->showSettings(SettingsCategory::Layouts);
}

void Ui::layoutSlotEvent(lv_event_t* event) {
    if (!instance_) return;
    const std::size_t index = static_cast<std::size_t>(
        reinterpret_cast<intptr_t>(lv_event_get_user_data(event)));
    instance_->openEditor({instance_->settings_flow_.layout(),
                           static_cast<uint8_t>(index)});
}

void Ui::spinDecreaseEvent(lv_event_t* event) {
    auto* spinbox = static_cast<lv_obj_t*>(lv_event_get_user_data(event));
    if (spinbox) lv_spinbox_decrement(spinbox);
}

void Ui::spinIncreaseEvent(lv_event_t* event) {
    auto* spinbox = static_cast<lv_obj_t*>(lv_event_get_user_data(event));
    if (spinbox) lv_spinbox_increment(spinbox);
}

void Ui::updateWarningModal(TileWarningEngine& warnings) {
    warning_engine_ = &warnings;
    const auto modal = warnings.nextModal();
    if (!modal.has_value()) {
        if (warning_panel_) { lv_obj_del(warning_panel_); warning_panel_ = nullptr; }
        return;
    }
    warning_address_ = modal->address;
    if (!warning_panel_) {
        warning_panel_ = lv_obj_create(lv_layer_top()); lv_obj_set_size(warning_panel_, 620, 300);
        lv_obj_center(warning_panel_); lv_obj_set_style_bg_color(warning_panel_, lv_color_hex(0x5A0909), 0);
        lv_obj_set_style_border_color(warning_panel_, UiTheme::red(), 0);
        lv_obj_set_style_border_width(warning_panel_, 5, 0);
        lv_obj_clear_flag(warning_panel_, LV_OBJ_FLAG_SCROLLABLE);
        warning_text_ = makeLabel(warning_panel_, "", 20, 20, &lv_font_montserrat_24, UiTheme::text());
        lv_obj_set_width(warning_text_, 570); lv_obj_set_style_text_align(warning_text_, LV_TEXT_ALIGN_CENTER, 0);
        makeButton(warning_panel_, "ACKNOWLEDGE", 180, 220, 260, 52, warningEvent);
    }
    const PresentedValue current = UnitPresenter::present(modal->parameter, modal->current_native, config_->units);
    const PresentedValue threshold = UnitPresenter::present(modal->parameter, modal->threshold_native, config_->units);
    char text[256];
    if (modal->remaining_count) {
        std::snprintf(text, sizeof(text), "WARNING\n%s\n%.1f %s\nLimit: %.1f %s\nMore warnings: %u",
            parameterDescriptor(modal->parameter).name, static_cast<double>(current.value), current.unit,
            static_cast<double>(threshold.value), threshold.unit,
            static_cast<unsigned>(modal->remaining_count));
    } else {
        std::snprintf(text, sizeof(text), "WARNING\n%s\n%.1f %s\nLimit: %.1f %s",
            parameterDescriptor(modal->parameter).name, static_cast<double>(current.value), current.unit,
            static_cast<double>(threshold.value), threshold.unit);
    }
    lv_label_set_text(warning_text_, text); lv_obj_move_foreground(warning_panel_);
}

void Ui::warningEvent(lv_event_t*) {
    if (!instance_ || !instance_->warning_engine_) return;
    instance_->warning_engine_->acknowledge(instance_->warning_address_);
    if (instance_->warning_panel_) {
        lv_obj_del(instance_->warning_panel_); instance_->warning_panel_ = nullptr;
    }
}
