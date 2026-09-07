#include "ui.h"
#include <cstdio>
#include <cstring>
#include "ui/tile_engine.h"
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
    BitrateChanged,
    TimeoutDecrease,
    TimeoutIncrease,
    ShiftStartDecrease,
    ShiftStartIncrease,
    ShiftRedDecrease,
    ShiftRedIncrease,
    ShiftMaxDecrease,
    ShiftMaxIncrease,
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
void Ui::begin(AppConfig& config, ConfigRepository& repository,
               BoardDisplay& board) {
    instance_ = this; config_ = &config; repository_ = &repository; board_ = &board;
    createDataPage(Page::Dash, config); createDataPage(Page::Track, config);
    board.setSoftwareBrightness(config.brightness_percent);
    update_policy_.takeLayoutDirty();
    load(Page::Dash);
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
    bitrate_dropdown_ = nullptr;
    can_timeout_ = nullptr;
    shift_start_ = nullptr;
    shift_red_ = nullptr;
    shift_max_ = nullptr;
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
    settings_message_ = makeLabel(settings_, "", 660, 27,
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
    if (previous && previous != settings_) lv_obj_del(previous);
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
    lv_dropdown_set_options(profile_dropdown_, "none");

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
    constexpr const char* labels[] = {"START RPM", "RED RPM", "MAX RPM"};
    lv_obj_t** boxes[] = {&shift_start_, &shift_red_, &shift_max_};
    const int32_t values[] = {config_->shift.start_rpm,
                              config_->shift.red_rpm,
                              config_->shift.max_rpm};
    const intptr_t decreases[] = {ShiftStartDecrease, ShiftRedDecrease,
                                  ShiftMaxDecrease};
    const intptr_t increases[] = {ShiftStartIncrease, ShiftRedIncrease,
                                  ShiftMaxIncrease};
    for (int index = 0; index < 3; ++index) {
        const int x = 34 + index * 250;
        makeLabel(panel, labels[index], x, 32, &lv_font_montserrat_14,
                  UiTheme::muted());
        *boxes[index] = makeSpinbox(panel, x, 68, 190, 1000, 15000,
                                    values[index], 5);
        lv_spinbox_set_step(*boxes[index], 100);
        makeButton(panel, "-", x, 128, 88, 48, settingsEvent,
                   reinterpret_cast<void*>(decreases[index]));
        makeButton(panel, "+", x + 102, 128, 88, 48, settingsEvent,
                   reinterpret_cast<void*>(increases[index]));
    }
    makeLabel(panel, "Values are saved after every change", 205, 235,
              &lv_font_montserrat_14, UiTheme::muted());
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
    makeButton(panel, "< PREVIOUS", 12, 286, 160, 44, layoutPageEvent,
               reinterpret_cast<void*>(-1));
    char page[24];
    std::snprintf(page, sizeof(page), "%u / %u",
                  static_cast<unsigned>(settings_flow_.pageIndex() + 1U),
                  static_cast<unsigned>(settings_flow_.pageCount()));
    makeLabel(panel, page, 360, 298, &lv_font_montserrat_14,
              UiTheme::text());
    makeButton(panel, "NEXT >", 596, 286, 160, 44, layoutPageEvent,
               reinterpret_cast<void*>(1));
}

void Ui::createSystemSettings(lv_obj_t* panel) {
    makeLabel(panel, "Firmware and runtime information", 24, 26,
              &lv_font_montserrat_14, UiTheme::text());
    settings_status_ = makeLabel(panel, "", 24, 78,
                                 &lv_font_montserrat_14, UiTheme::muted());
    makeLabel(panel, "Reset controls require confirmation", 24, 270,
              &lv_font_montserrat_14, UiTheme::yellow());
}

void Ui::update(const VehicleState& state, const RuntimeDiagnostics& diagnostics,
                const UiRuntimeStatus& status, const AppConfig& config,
                TileWarningEngine& warnings) {
    if (update_policy_.takeLayoutDirty()) {
        applyLayout(Page::Dash, config);
        applyLayout(Page::Track, config);
    }
    const SignalValue& rpm = state.get(ParameterId::Rpm);
    const uint16_t rpm_value = rpm.valid && rpm.value > 0.0f ? static_cast<uint16_t>(rpm.value) : 0U;
    if (update_policy_.shouldUpdateData(PageId::Dash)) {
        for (std::size_t i = 0U; i < dash_tiles_.size(); ++i)
            dash_tiles_[i].update(config.dash_tiles[i], config.units, state,
                warnings.isHighlighted({PageId::Dash, static_cast<uint8_t>(i)}));
        dash_shift_.update(rpm_value, config.shift);
    }
    if (update_policy_.shouldUpdateData(PageId::Track)) {
        for (std::size_t i = 0U; i < track_tiles_.size(); ++i)
            track_tiles_[i].update(config.track_tiles[i], config.units, state,
                warnings.isHighlighted({PageId::Track, static_cast<uint8_t>(i)}));
        track_shift_.update(rpm_value, config.shift);
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
    if (update_policy_.allowModalUpdates()) {
        updateWarningModal(warnings);
    }
}
void Ui::navEvent(lv_event_t* event) {
    if (!instance_) return;
    instance_->load(static_cast<Page>(reinterpret_cast<intptr_t>(lv_event_get_user_data(event))));
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

bool Ui::takeRuntimeReconfigureRequest() {
    const bool requested = runtime_reconfigure_requested_;
    runtime_reconfigure_requested_ = false;
    return requested;
}

void Ui::openEditor(TileAddress address) {
    if (!config_ || !editor_.open(address, *config_)) return;
    closeEditor();
    editor_.open(address, *config_);
    const TileConfig& tile = editor_.draft().tile;
    editor_overlay_ = lv_obj_create(lv_layer_top());
    lv_obj_set_size(editor_overlay_, 720, 410); lv_obj_center(editor_overlay_);
    lv_obj_set_style_bg_color(editor_overlay_, UiTheme::panel(), 0);
    lv_obj_set_style_border_color(editor_overlay_, UiTheme::blue(), 0);
    lv_obj_set_style_border_width(editor_overlay_, 2, 0);
    lv_obj_clear_flag(editor_overlay_, LV_OBJ_FLAG_SCROLLABLE);
    makeLabel(editor_overlay_, "TILE SETTINGS", 20, 14, &lv_font_montserrat_24, UiTheme::text());

    makeLabel(editor_overlay_, "Parameter", 20, 64, &lv_font_montserrat_12, UiTheme::muted());
    editor_parameter_ = lv_dropdown_create(editor_overlay_); lv_obj_set_pos(editor_parameter_, 20, 84);
    lv_obj_set_size(editor_parameter_, 250, 42);
    lv_dropdown_set_options(editor_parameter_,
        "RPM\nMAP\nLAMBDA\nTPS\nCLT\nIAT\nOIL PRESS\nOIL TEMP\nBATTERY\nSPEED\nGEAR\nFUEL PRESS");
    lv_dropdown_set_selected(editor_parameter_, static_cast<uint16_t>(tile.parameter));
    editor_visible_ = lv_checkbox_create(editor_overlay_); lv_obj_set_pos(editor_visible_, 300, 92);
    lv_checkbox_set_text(editor_visible_, "Visible");
    if (tile.visible) lv_obj_add_state(editor_visible_, LV_STATE_CHECKED);
    makeLabel(editor_overlay_, "Decimals", 480, 64, &lv_font_montserrat_12, UiTheme::muted());
    editor_decimals_ = lv_dropdown_create(editor_overlay_); lv_obj_set_pos(editor_decimals_, 480, 84);
    lv_obj_set_size(editor_decimals_, 150, 42); lv_dropdown_set_options(editor_decimals_, "0\n1\n2\n3");
    lv_dropdown_set_selected(editor_decimals_, tile.decimals);

    editor_warning_ = lv_checkbox_create(editor_overlay_); lv_obj_set_pos(editor_warning_, 20, 152);
    lv_checkbox_set_text(editor_warning_, "Enable WARNING");
    if (tile.warning.enabled) lv_obj_add_state(editor_warning_, LV_STATE_CHECKED);
    makeLabel(editor_overlay_, "Direction", 20, 194, &lv_font_montserrat_12, UiTheme::muted());
    editor_direction_ = lv_dropdown_create(editor_overlay_); lv_obj_set_pos(editor_direction_, 20, 214);
    lv_obj_set_size(editor_direction_, 150, 42); lv_dropdown_set_options(editor_direction_, "Above\nBelow");
    lv_dropdown_set_selected(editor_direction_, static_cast<uint16_t>(tile.warning.direction));
    makeLabel(editor_overlay_, "Threshold", 190, 194, &lv_font_montserrat_12, UiTheme::muted());
    editor_threshold_ = makeSpinbox(editor_overlay_, 190, 214, 150, -99999, 99999,
        static_cast<int32_t>(tile.warning.threshold_native * 100.0f), 5, 2);
    lv_spinbox_set_step(editor_threshold_, 10); makeStepper(editor_overlay_, editor_threshold_, 190, 260, 65);
    makeLabel(editor_overlay_, "Hysteresis", 360, 194, &lv_font_montserrat_12, UiTheme::muted());
    editor_hysteresis_ = makeSpinbox(editor_overlay_, 360, 214, 150, 0, 99999,
        static_cast<int32_t>(tile.warning.hysteresis_native * 100.0f), 5, 2);
    lv_spinbox_set_step(editor_hysteresis_, 10); makeStepper(editor_overlay_, editor_hysteresis_, 360, 260, 65);
    makeLabel(editor_overlay_, "Delay ms", 530, 194, &lv_font_montserrat_12, UiTheme::muted());
    editor_delay_ = makeSpinbox(editor_overlay_, 530, 214, 150, 0, 10000,
        tile.warning.delay_ms, 5);
    lv_spinbox_set_step(editor_delay_, 100); makeStepper(editor_overlay_, editor_delay_, 530, 260, 65);
    makeLabel(editor_overlay_, "Use − / + to adjust threshold, hysteresis and delay.", 20, 304,
              &lv_font_montserrat_12, UiTheme::muted());
    makeButton(editor_overlay_, "CANCEL", 20, 326, 180, 48, editorEvent,
               reinterpret_cast<void*>(1));
    makeButton(editor_overlay_, "SAVE TILE", 500, 326, 180, 48, editorEvent,
               reinterpret_cast<void*>(2));
    editor_message_ = makeLabel(editor_overlay_, "", 225, 342,
                                &lv_font_montserrat_14, UiTheme::red());
}

void Ui::closeEditor() {
    if (editor_overlay_) { lv_obj_del(editor_overlay_); editor_overlay_ = nullptr; }
    editor_.cancel();
}

void Ui::saveEditor() {
    if (!config_ || !repository_ || !editor_.isOpen()) return;
    editor_.setParameter(static_cast<ParameterId>(lv_dropdown_get_selected(editor_parameter_)));
    editor_.setVisible(lv_obj_has_state(editor_visible_, LV_STATE_CHECKED));
    editor_.setDecimals(static_cast<uint8_t>(lv_dropdown_get_selected(editor_decimals_)));
    TileWarningConfig warning;
    warning.enabled = lv_obj_has_state(editor_warning_, LV_STATE_CHECKED);
    warning.direction = static_cast<WarningDirection>(lv_dropdown_get_selected(editor_direction_));
    warning.threshold_native = static_cast<float>(lv_spinbox_get_value(editor_threshold_)) / 100.0f;
    warning.hysteresis_native = static_cast<float>(lv_spinbox_get_value(editor_hysteresis_)) / 100.0f;
    warning.delay_ms = static_cast<uint16_t>(lv_spinbox_get_value(editor_delay_));
    editor_.setWarning(warning);
    AppConfig candidate = *config_;
    if (!editor_.applyTo(candidate) || !repository_->saveCandidate(candidate, *config_)) {
        lv_label_set_text(editor_message_, "SAVE FAILED"); return;
    }
    runtime_reconfigure_requested_ = true;
    update_policy_.markLayoutDirty();
    const bool refresh_layout = current_page_ == Page::Settings &&
        settings_flow_.category() == SettingsCategory::Layouts;
    closeEditor();
    if (refresh_layout) showSettings(SettingsCategory::Layouts);
}

void Ui::updateSettingsControls() {
    if (!config_ || !brightness_slider_) return;
    lv_slider_set_value(brightness_slider_, config_->brightness_percent, LV_ANIM_OFF);
    lv_dropdown_set_selected(source_dropdown_, config_->data_source == DataSource::Can ? 1 : 0);
    uint16_t bitrate_index = config_->can.bitrate == 125000U ? 0U :
        (config_->can.bitrate == 250000U ? 1U : (config_->can.bitrate == 500000U ? 2U : 3U));
    lv_dropdown_set_selected(bitrate_dropdown_, bitrate_index);
    lv_spinbox_set_value(shift_start_, config_->shift.start_rpm);
    lv_spinbox_set_value(shift_red_, config_->shift.red_rpm);
    lv_spinbox_set_value(shift_max_, config_->shift.max_rpm);
    lv_spinbox_set_value(can_timeout_, config_->can.timeout_ms);
    lv_dropdown_set_selected(temp_unit_, static_cast<uint16_t>(config_->units.temperature));
    lv_dropdown_set_selected(pressure_unit_, static_cast<uint16_t>(config_->units.pressure));
    lv_dropdown_set_selected(speed_unit_, static_cast<uint16_t>(config_->units.speed));
    lv_dropdown_set_selected(mixture_unit_, static_cast<uint16_t>(config_->units.mixture));
    for (std::size_t i = 0U; i < layout_labels_.size(); ++i) {
        const bool track = i >= AppConfig::kDashTileCount;
        const std::size_t slot = track ? i - AppConfig::kDashTileCount : i;
        const TileConfig& tile = track ? config_->track_tiles[slot] : config_->dash_tiles[slot];
        char text[96]; std::snprintf(text, sizeof(text), "Slot %u • %s • %s",
            static_cast<unsigned>(slot + 1U), parameterDescriptor(tile.parameter).name,
            tile.visible ? "VISIBLE" : "HIDDEN");
        lv_label_set_text(layout_labels_[i], text);
    }
}

void Ui::showSettingsMessage(const char* message) {
    if (settings_message_) lv_label_set_text(settings_message_, message);
}

bool Ui::persistSettings(AppConfig candidate, bool reconfigure_runtime) {
    if (!repository_ || !config_ || !candidate.validate().valid ||
        !repository_->saveCandidate(candidate, *config_)) {
        showSettingsMessage("SAVE ERROR");
        if (board_ && config_)
            board_->setSoftwareBrightness(config_->brightness_percent);
        return false;
    }
    runtime_reconfigure_requested_ |= reconfigure_runtime;
    if (board_) board_->setSoftwareBrightness(config_->brightness_percent);
    showSettingsMessage("SAVED");
    return true;
}

void Ui::saveSettings() {
    if (!config_ || !repository_) return;
    AppConfig candidate = *config_;
    candidate.brightness_percent = static_cast<uint8_t>(lv_slider_get_value(brightness_slider_));
    candidate.data_source = lv_dropdown_get_selected(source_dropdown_) == 1U
                                ? DataSource::Can : DataSource::Demo;
    constexpr uint32_t bitrates[] = {125000U, 250000U, 500000U, 1000000U};
    candidate.can.bitrate = bitrates[lv_dropdown_get_selected(bitrate_dropdown_)];
    candidate.can.timeout_ms = static_cast<uint32_t>(lv_spinbox_get_value(can_timeout_));
    candidate.shift.start_rpm = static_cast<uint16_t>(lv_spinbox_get_value(shift_start_));
    candidate.shift.red_rpm = static_cast<uint16_t>(lv_spinbox_get_value(shift_red_));
    candidate.shift.max_rpm = static_cast<uint16_t>(lv_spinbox_get_value(shift_max_));
    candidate.units.temperature = static_cast<TemperatureUnit>(lv_dropdown_get_selected(temp_unit_));
    candidate.units.pressure = static_cast<PressureUnit>(lv_dropdown_get_selected(pressure_unit_));
    candidate.units.speed = static_cast<SpeedUnit>(lv_dropdown_get_selected(speed_unit_));
    candidate.units.mixture = static_cast<MixtureUnit>(lv_dropdown_get_selected(mixture_unit_));
    if (!candidate.validate().valid) {
        lv_label_set_text(settings_message_, "Required: start < red <= max"); return;
    }
    if (!repository_->saveCandidate(candidate, *config_)) {
        lv_label_set_text(settings_message_, "SAVE FAILED");
        board_->setSoftwareBrightness(config_->brightness_percent); return;
    }
    runtime_reconfigure_requested_ = true; board_->setSoftwareBrightness(config_->brightness_percent);
    lv_label_set_text(settings_message_, "SAVED"); updateSettingsControls();
}

void Ui::resetLayouts() {
    if (!config_ || !repository_) return;
    AppConfig candidate = *config_; const AppConfig defaults = AppConfig::defaults();
    candidate.dash_tiles = defaults.dash_tiles; candidate.track_tiles = defaults.track_tiles;
    if (repository_->saveCandidate(candidate, *config_)) {
        update_policy_.markLayoutDirty();
        lv_label_set_text(settings_message_, "LAYOUTS RESET"); updateSettingsControls();
    } else lv_label_set_text(settings_message_, "RESET FAILED");
}

void Ui::factoryReset() {
    if (!config_ || !repository_) return;
    if (!repository_->reset(*config_)) { lv_label_set_text(settings_message_, "RESET FAILED"); return; }
    runtime_reconfigure_requested_ = true; updateSettingsControls();
    update_policy_.markLayoutDirty();
    board_->setSoftwareBrightness(config_->brightness_percent); load(Page::Dash);
}

void Ui::editorEvent(lv_event_t* event) {
    if (!instance_) return;
    const intptr_t action = reinterpret_cast<intptr_t>(lv_event_get_user_data(event));
    if (action == 1) instance_->closeEditor();
    if (action == 2) instance_->saveEditor();
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
        instance_->persistSettings(candidate, false);
        return;
    }
    if (action == SourceChanged) {
        candidate.data_source = lv_dropdown_get_selected(
            instance_->source_dropdown_) == 1U ? DataSource::Can
                                                : DataSource::Demo;
        instance_->persistSettings(candidate, true);
        return;
    }
    if (action == BitrateChanged) {
        constexpr uint32_t bitrates[] = {125000U, 250000U, 500000U, 1000000U};
        candidate.can.bitrate = bitrates[lv_dropdown_get_selected(
            instance_->bitrate_dropdown_)];
        instance_->persistSettings(candidate, true);
        return;
    }
    if (action == TimeoutDecrease || action == TimeoutIncrease) {
        if (action == TimeoutDecrease)
            lv_spinbox_decrement(instance_->can_timeout_);
        else
            lv_spinbox_increment(instance_->can_timeout_);
        candidate.can.timeout_ms = static_cast<uint32_t>(
            lv_spinbox_get_value(instance_->can_timeout_));
        instance_->persistSettings(candidate, true);
        return;
    }

    ShiftField field = ShiftField::Start;
    lv_obj_t* box = nullptr;
    bool increase = false;
    if (action == ShiftStartDecrease || action == ShiftStartIncrease) {
        field = ShiftField::Start; box = instance_->shift_start_;
        increase = action == ShiftStartIncrease;
    } else if (action == ShiftRedDecrease || action == ShiftRedIncrease) {
        field = ShiftField::Red; box = instance_->shift_red_;
        increase = action == ShiftRedIncrease;
    } else if (action == ShiftMaxDecrease || action == ShiftMaxIncrease) {
        field = ShiftField::Maximum; box = instance_->shift_max_;
        increase = action == ShiftMaxIncrease;
    }
    if (box) {
        if (increase) lv_spinbox_increment(box); else lv_spinbox_decrement(box);
        candidate.shift = SettingsFlowModel::correctedShift(
            candidate.shift, field,
            static_cast<uint16_t>(lv_spinbox_get_value(box)));
        lv_spinbox_set_value(instance_->shift_start_, candidate.shift.start_rpm);
        lv_spinbox_set_value(instance_->shift_red_, candidate.shift.red_rpm);
        lv_spinbox_set_value(instance_->shift_max_, candidate.shift.max_rpm);
        instance_->persistSettings(candidate, false);
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
        instance_->persistSettings(candidate, false);
    }
}

void Ui::settingsCategoryEvent(lv_event_t* event) {
    if (!instance_) return;
    instance_->showSettings(static_cast<SettingsCategory>(
        reinterpret_cast<intptr_t>(lv_event_get_user_data(event))));
}

void Ui::settingsBackEvent(lv_event_t*) {
    if (!instance_) return;
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
        std::snprintf(text, sizeof(text), "WARNING\n%s\n%.2f %s\nLimit: %.2f %s\nMore warnings: %u",
            parameterDescriptor(modal->parameter).name, static_cast<double>(current.value), current.unit,
            static_cast<double>(threshold.value), threshold.unit,
            static_cast<unsigned>(modal->remaining_count));
    } else {
        std::snprintf(text, sizeof(text), "WARNING\n%s\n%.2f %s\nLimit: %.2f %s",
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
