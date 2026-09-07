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
    createSettings(); board.setSoftwareBrightness(config.brightness_percent);
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
void Ui::createSettings() {
    settings_ = lv_obj_create(nullptr); styleScreen(settings_);
    makeLabel(settings_, "SETTINGS", 24, 20, &lv_font_montserrat_28, UiTheme::text());
    settings_status_ = makeLabel(settings_, "---", 190, 27, &lv_font_montserrat_12,
                                 UiTheme::muted());
    settings_container_ = lv_obj_create(settings_); lv_obj_set_pos(settings_container_, 16, 66);
    lv_obj_set_size(settings_container_, 768, 348);
    lv_obj_set_style_bg_color(settings_container_, UiTheme::panel(), 0);
    lv_obj_set_style_border_color(settings_container_, UiTheme::border(), 0);
    lv_obj_set_scroll_dir(settings_container_, LV_DIR_VER);

    makeLabel(settings_container_, "DISPLAY", 12, 10, &lv_font_montserrat_14, UiTheme::blue());
    makeLabel(settings_container_, "Brightness (20-100%)", 12, 39, &lv_font_montserrat_14, UiTheme::text());
    brightness_slider_ = lv_slider_create(settings_container_);
    lv_obj_set_pos(brightness_slider_, 220, 42); lv_obj_set_size(brightness_slider_, 500, 16);
    lv_slider_set_range(brightness_slider_, 20, 100);
    lv_obj_add_event_cb(brightness_slider_, settingsEvent, LV_EVENT_VALUE_CHANGED,
                        reinterpret_cast<void*>(1));

    makeLabel(settings_container_, "DATA SOURCE & CAN", 12, 82, &lv_font_montserrat_14, UiTheme::blue());
    source_dropdown_ = lv_dropdown_create(settings_container_); lv_obj_set_pos(source_dropdown_, 12, 111);
    lv_obj_set_size(source_dropdown_, 180, 40); lv_dropdown_set_options(source_dropdown_, "DEMO\nCAN");
    bitrate_dropdown_ = lv_dropdown_create(settings_container_); lv_obj_set_pos(bitrate_dropdown_, 210, 111);
    lv_obj_set_size(bitrate_dropdown_, 210, 40);
    lv_dropdown_set_options(bitrate_dropdown_, "125 kbit/s\n250 kbit/s\n500 kbit/s\n1000 kbit/s");
    makeLabel(settings_container_, "Timeout ms", 440, 93, &lv_font_montserrat_12, UiTheme::muted());
    can_timeout_ = makeSpinbox(settings_container_, 440, 111, 150, 100, 5000, 500, 4);
    lv_spinbox_set_step(can_timeout_, 100); makeStepper(settings_container_, can_timeout_, 600, 114, 48);
    makeLabel(settings_container_, "Receive-only • profile: none", 440, 155,
              &lv_font_montserrat_12, UiTheme::muted());

    makeLabel(settings_container_, "SHIFT LIGHTS", 12, 174, &lv_font_montserrat_14, UiTheme::blue());
    makeLabel(settings_container_, "Start", 12, 207, &lv_font_montserrat_12, UiTheme::muted());
    shift_start_ = makeSpinbox(settings_container_, 62, 194, 150, 1000, 15000, 5500, 5);
    lv_spinbox_set_step(shift_start_, 100); makeStepper(settings_container_, shift_start_, 62, 236, 65);
    makeLabel(settings_container_, "Red", 235, 207, &lv_font_montserrat_12, UiTheme::muted());
    shift_red_ = makeSpinbox(settings_container_, 275, 194, 150, 1000, 15000, 7000, 5);
    lv_spinbox_set_step(shift_red_, 100); makeStepper(settings_container_, shift_red_, 275, 236, 65);
    makeLabel(settings_container_, "Max", 448, 207, &lv_font_montserrat_12, UiTheme::muted());
    shift_max_ = makeSpinbox(settings_container_, 488, 194, 150, 1000, 15000, 8000, 5);
    lv_spinbox_set_step(shift_max_, 100); makeStepper(settings_container_, shift_max_, 488, 236, 65);

    makeLabel(settings_container_, "UNITS", 12, 292, &lv_font_montserrat_14, UiTheme::blue());
    temp_unit_ = lv_dropdown_create(settings_container_); lv_obj_set_pos(temp_unit_, 12, 320);
    lv_obj_set_size(temp_unit_, 165, 40); lv_dropdown_set_options(temp_unit_, "Celsius\nFahrenheit");
    pressure_unit_ = lv_dropdown_create(settings_container_); lv_obj_set_pos(pressure_unit_, 190, 320);
    lv_obj_set_size(pressure_unit_, 165, 40); lv_dropdown_set_options(pressure_unit_, "bar\nkPa\npsi");
    speed_unit_ = lv_dropdown_create(settings_container_); lv_obj_set_pos(speed_unit_, 368, 320);
    lv_obj_set_size(speed_unit_, 165, 40); lv_dropdown_set_options(speed_unit_, "km/h\nmph");
    mixture_unit_ = lv_dropdown_create(settings_container_); lv_obj_set_pos(mixture_unit_, 546, 320);
    lv_obj_set_size(mixture_unit_, 165, 40); lv_dropdown_set_options(mixture_unit_, "lambda\nAFR");

    makeButton(settings_container_, "SAVE SETTINGS", 12, 380, 220, 46, settingsEvent,
               reinterpret_cast<void*>(2));
    settings_message_ = makeLabel(settings_container_, "", 250, 394,
                                  &lv_font_montserrat_14, UiTheme::yellow());
    makeLabel(settings_container_, "LAYOUTS — tap any slot, including hidden ones", 12, 455,
              &lv_font_montserrat_14, UiTheme::blue());
    int y = 488;
    for (std::size_t i = 0U; i < layout_labels_.size(); ++i) {
        const bool track = i >= AppConfig::kDashTileCount;
        const std::size_t slot = track ? i - AppConfig::kDashTileCount : i;
        makeLabel(settings_container_, track ? "TRACK" : "DASH", 12, y + 13,
                  &lv_font_montserrat_12, UiTheme::muted());
        lv_obj_t* button = makeButton(settings_container_, "", 88, y, 620, 40,
                                      layoutSlotEvent,
                                      reinterpret_cast<void*>(static_cast<intptr_t>(i)));
        layout_labels_[i] = lv_obj_get_child(button, 0);
        y += 48;
    }
    makeButton(settings_container_, "RESET LAYOUTS", 12, y + 10, 220, 46,
               settingsEvent, reinterpret_cast<void*>(3));
    makeButton(settings_container_, "FACTORY RESET", 250, y + 10, 220, 46,
               settingsEvent, reinterpret_cast<void*>(4));
    lv_obj_set_height(settings_container_, 348);
    createNavigation(settings_, Page::Settings);
    updateSettingsControls();
}
void Ui::update(const VehicleState& state, const RuntimeDiagnostics&,
                const UiRuntimeStatus& status, const AppConfig& config,
                TileWarningEngine& warnings) {
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
    updateWarningModal(warnings);
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
    if (page == Page::Dash) lv_scr_load(dash_);
    if (page == Page::Track) lv_scr_load(track_);
    if (page == Page::Settings) lv_scr_load(settings_);
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
    updateSettingsControls(); closeEditor();
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
        lv_label_set_text(settings_message_, "LAYOUTS RESET"); updateSettingsControls();
    } else lv_label_set_text(settings_message_, "RESET FAILED");
}

void Ui::factoryReset() {
    if (!config_ || !repository_) return;
    if (!repository_->reset(*config_)) { lv_label_set_text(settings_message_, "RESET FAILED"); return; }
    runtime_reconfigure_requested_ = true; updateSettingsControls();
    board_->setSoftwareBrightness(config_->brightness_percent); load(Page::Dash);
}

void Ui::editorEvent(lv_event_t* event) {
    if (!instance_) return;
    const intptr_t action = reinterpret_cast<intptr_t>(lv_event_get_user_data(event));
    if (action == 1) instance_->closeEditor();
    if (action == 2) instance_->saveEditor();
}

void Ui::settingsEvent(lv_event_t* event) {
    if (!instance_) return;
    const intptr_t action = reinterpret_cast<intptr_t>(lv_event_get_user_data(event));
    if (action == 1 && instance_->board_)
        instance_->board_->setSoftwareBrightness(static_cast<uint8_t>(
            lv_slider_get_value(instance_->brightness_slider_)));
    if (action == 2) instance_->saveSettings();
    if (action == 3) instance_->resetLayouts();
    if (action == 4) instance_->factoryReset();
}

void Ui::layoutSlotEvent(lv_event_t* event) {
    if (!instance_) return;
    const std::size_t index = static_cast<std::size_t>(
        reinterpret_cast<intptr_t>(lv_event_get_user_data(event)));
    if (index < AppConfig::kDashTileCount)
        instance_->openEditor({PageId::Dash, static_cast<uint8_t>(index)});
    else instance_->openEditor({PageId::Track,
        static_cast<uint8_t>(index - AppConfig::kDashTileCount)});
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
