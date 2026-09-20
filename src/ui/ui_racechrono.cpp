#include "ui.h"

#include <algorithm>
#include <cstdio>

#include "racechrono/racechrono_channel_catalog.h"
#include "telemetry/parameter_registry.h"
#include "ui/editor_widgets.h"
#include "ui/ui_theme.h"
#include "ui/unit_presenter.h"

using namespace EditorWidgets;

namespace {

enum RaceChronoUiAction : intptr_t {
    ShowConnection = 100,
    ShowChannels,
    ToggleEnabled,
    ToggleEnabledRow,
    RestartBle,
    ChangeFilter,
    PreviousPage,
    NextPage,
};

const char* channelStateText(RaceChronoChannelState state) {
    switch (state) {
        case RaceChronoChannelState::Active: return "ACTIVE";
        case RaceChronoChannelState::Error: return "ERROR";
        case RaceChronoChannelState::NoData: return "NO DATA";
    }
    return "NO DATA";
}

lv_color_t channelStateColor(RaceChronoChannelState state) {
    if (state == RaceChronoChannelState::Active) return UiTheme::green();
    if (state == RaceChronoChannelState::Error) return UiTheme::red();
    return UiTheme::muted();
}

lv_obj_t* framedPanel(lv_obj_t* parent, int x, int y, int width,
                      int height) {
    lv_obj_t* object = panel(parent, x, y, width, height);
    lv_obj_set_style_bg_color(object, lv_color_hex(0x10151B), 0);
    lv_obj_set_style_border_color(object, UiTheme::border(), 0);
    lv_obj_set_style_border_width(object, 1, 0);
    lv_obj_set_style_radius(object, 7, 0);
    return object;
}

lv_obj_t* clippedLabel(lv_obj_t* parent, const char* text, int x, int y,
                       int width, lv_color_t color = UiTheme::text()) {
    lv_obj_t* object = label(parent, text, x, y, &lv_font_montserrat_12,
                             color);
    lv_obj_set_width(object, width);
    lv_label_set_long_mode(object, LV_LABEL_LONG_DOT);
    return object;
}

void addSeparator(lv_obj_t* parent, int y, int width) {
    lv_obj_t* line = panel(parent, 0, y, width, 1);
    lv_obj_set_style_bg_color(line, lv_color_hex(0x283139), 0);
}

}  // namespace

void Ui::createRaceChronoSettings(lv_obj_t* root) {
    const AppConfig& settings = settingsConfig();
    racechrono_settings_.setChannelCount(raceChronoChannelCount());
    for (std::size_t index = 0U; index < raceChronoChannelCount(); ++index) {
        racechrono_settings_.setChannelState(
            index, latest_racechrono_.channelState(
                       raceChronoChannelAt(index).parameter));
    }

    lv_obj_t* connection_tab = button(
        root, "CONNECTION", 8, 0, 370, 40, raceChronoEvent,
        ShowConnection, !racechrono_channels_tab_);
    lv_obj_t* channels_tab = button(
        root, "CHANNELS", 390, 0, 370, 40, raceChronoEvent,
        ShowChannels, racechrono_channels_tab_);
    lv_obj_set_style_border_color(connection_tab,
        racechrono_channels_tab_ ? UiTheme::border() : UiTheme::blue(), 0);
    lv_obj_set_style_border_color(channels_tab,
        racechrono_channels_tab_ ? UiTheme::blue() : UiTheme::border(), 0);

    lv_obj_t* content = framedPanel(root, 8, 50, 752, 286);
    if (!racechrono_channels_tab_) {
        lv_obj_t* enabled_row = lv_obj_create(content);
        lv_obj_set_pos(enabled_row, 0, 0);
        lv_obj_set_size(enabled_row, 650, 42);
        lv_obj_set_style_bg_opa(enabled_row, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(enabled_row, 0, 0);
        lv_obj_set_style_pad_all(enabled_row, 0, 0);
        lv_obj_clear_flag(enabled_row, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_add_flag(enabled_row, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(enabled_row, raceChronoEvent, LV_EVENT_CLICKED,
                            reinterpret_cast<void*>(ToggleEnabledRow));
        label(enabled_row, "ENABLED", 16, 14,
              &lv_font_montserrat_12, UiTheme::muted());

        constexpr const char* names[] = {
            "BLE STATUS", "DATA STATUS", "GPS STATUS"};
        for (int row = 1; row < 4; ++row) {
            label(content, names[row - 1], 16, row * 42 + 14,
                  &lv_font_montserrat_12, UiTheme::muted());
            addSeparator(content, (row + 1) * 42 - 1, 750);
        }
        addSeparator(content, 41, 750);

        racechrono_enabled_ = lv_switch_create(content);
        lv_obj_set_pos(racechrono_enabled_, 670, 5);
        lv_obj_set_size(racechrono_enabled_, 64, 32);
        lv_obj_set_style_bg_color(racechrono_enabled_, UiTheme::border(),
                                  LV_PART_MAIN);
        lv_obj_set_style_bg_color(racechrono_enabled_, UiTheme::blue(),
                                  LV_PART_INDICATOR | LV_STATE_CHECKED);
        lv_obj_set_style_bg_color(racechrono_enabled_, UiTheme::text(),
                                  LV_PART_KNOB);
        if (settings.racechrono.enabled)
            lv_obj_add_state(racechrono_enabled_, LV_STATE_CHECKED);
        lv_obj_add_event_cb(racechrono_enabled_, raceChronoEvent,
                            LV_EVENT_VALUE_CHANGED,
                            reinterpret_cast<void*>(ToggleEnabled));

        racechrono_ble_status_ = clippedLabel(
            content, "", 430, 56, 286, UiTheme::muted());
        racechrono_data_status_ = clippedLabel(
            content, "", 430, 98, 286, UiTheme::muted());
        racechrono_gps_status_ = clippedLabel(
            content, "", 430, 140, 286, UiTheme::muted());
        lv_obj_t* status_labels[] = {racechrono_ble_status_,
                                     racechrono_data_status_,
                                     racechrono_gps_status_};
        for (lv_obj_t* status : status_labels) {
            lv_obj_set_style_text_align(status, LV_TEXT_ALIGN_RIGHT, 0);
        }

        constexpr const char* summary_names[] = {
            "BLE PACKETS", "ACTIVE CHANNELS", "SATELLITES", "GPS ACCURACY"};
        lv_obj_t** summary_values[] = {
            &racechrono_packets_, &racechrono_active_,
            &racechrono_satellites_, &racechrono_accuracy_};
        for (int index = 0; index < 4; ++index) {
            const int column = index % 2;
            const int row = index / 2;
            const int x = 16 + column * 370;
            label(content, summary_names[index], x, 177 + row * 26,
                  &lv_font_montserrat_12, UiTheme::muted());
            *summary_values[index] = clippedLabel(
                content, "", x + 174, 177 + row * 26, 160);
            lv_obj_set_style_text_align(*summary_values[index],
                                        LV_TEXT_ALIGN_RIGHT, 0);
        }
        addSeparator(content, 227, 750);
        clippedLabel(content, "DEVICE: DIY DASH RC", 16, 250, 480,
                     UiTheme::muted());
        button(content, "RESTART BLE", 568, 237, 166, 38,
               raceChronoEvent, RestartBle);
    } else {
        label(content, "CHANNEL FILTER", 16, 14,
              &lv_font_montserrat_12, UiTheme::muted());
        lv_obj_t* filter = lv_dropdown_create(content);
        lv_obj_set_pos(filter, 498, 5);
        lv_obj_set_size(filter, 236, 34);
        lv_dropdown_set_options(filter, "ALL\nACTIVE\nNO DATA\nERROR");
        lv_dropdown_set_selected(filter, static_cast<uint16_t>(
            racechrono_settings_.filter()));
        darkDropdown(filter);
        lv_obj_add_event_cb(filter, raceChronoEvent, LV_EVENT_VALUE_CHANGED,
                            reinterpret_cast<void*>(ChangeFilter));
        addSeparator(content, 41, 750);

        for (std::size_t row = 0U;
             row < RaceChronoSettingsModel::kRowsPerPage; ++row) {
            const int y = 42 + static_cast<int>(row) * 32;
            racechrono_channel_names_[row] = clippedLabel(
                content, "", 16, y + 10, 330);
            racechrono_channel_states_[row] = clippedLabel(
                content, "", 360, y + 10, 130, UiTheme::muted());
            racechrono_channel_values_[row] = clippedLabel(
                content, "", 500, y + 10, 218);
            lv_label_set_long_mode(racechrono_channel_names_[row],
                                   LV_LABEL_LONG_CLIP);
            lv_label_set_long_mode(racechrono_channel_states_[row],
                                   LV_LABEL_LONG_CLIP);
            lv_label_set_long_mode(racechrono_channel_values_[row],
                                   LV_LABEL_LONG_CLIP);
            lv_obj_set_style_text_align(racechrono_channel_values_[row],
                                        LV_TEXT_ALIGN_RIGHT, 0);
            addSeparator(content, y + 31, 750);
        }

        racechrono_previous_page_ = button(
            content, "< PREVIOUS", 16, 243, 150, 34,
            raceChronoEvent, PreviousPage);
        if (racechrono_settings_.pageIndex() == 0U)
            lv_obj_add_state(racechrono_previous_page_, LV_STATE_DISABLED);
        char page_text[32];
        std::snprintf(page_text, sizeof(page_text), "PAGE %u / %u",
            static_cast<unsigned>(racechrono_settings_.pageIndex() + 1U),
            static_cast<unsigned>(racechrono_settings_.pageCount()));
        racechrono_page_number_ = label(content, page_text, 276, 254,
                                        &lv_font_montserrat_12,
                                        UiTheme::muted());
        lv_obj_set_width(racechrono_page_number_, 200);
        lv_obj_set_style_text_align(racechrono_page_number_,
                                    LV_TEXT_ALIGN_CENTER, 0);
        racechrono_next_page_ = button(content, "NEXT >", 584, 243, 150, 34,
                                       raceChronoEvent, NextPage);
        if (racechrono_settings_.pageIndex() + 1U >=
            racechrono_settings_.pageCount())
            lv_obj_add_state(racechrono_next_page_, LV_STATE_DISABLED);
    }
    refreshRaceChronoSettings();
}

void Ui::refreshRaceChronoSettings() {
    for (std::size_t index = 0U; index < raceChronoChannelCount(); ++index) {
        racechrono_settings_.setChannelState(
            index, latest_racechrono_.channelState(
                       raceChronoChannelAt(index).parameter));
    }

    const RaceChronoRuntimeStatus& status = latest_ui_status_.racechrono;
    const RaceChronoConnectionState connection =
        latest_ui_status_.racechrono_connection;
    const bool enabled = settingsConfig().racechrono.enabled;
    const bool connection_error =
        enabled && connection == RaceChronoConnectionState::Error;
    const bool connected = enabled &&
        (connection == RaceChronoConnectionState::Connected ||
         connection == RaceChronoConnectionState::Configuring ||
         connection == RaceChronoConnectionState::Active ||
         connection == RaceChronoConnectionState::NoData);
    const bool active = enabled && status.has_valid_packet;
    const bool has_fix = active && status.gps_fix_type >= 2U;
    auto set_status = [](lv_obj_t* object, const char* text,
                         lv_color_t color) {
        if (!object) return;
        lv_label_set_text(object, text);
        lv_obj_set_style_text_color(object, color, 0);
    };
    const char* ble_text = !enabled ? "BLE DISABLED" :
        connection_error ? "BLE: ERROR" :
        connected ? "BLE: CONNECTED" :
        connection == RaceChronoConnectionState::Advertising
            ? "BLE: WAITING FOR APP" : "BLE: STARTING";
    set_status(racechrono_ble_status_, ble_text,
               connection_error ? UiTheme::red() :
               connected ? UiTheme::green() : UiTheme::muted());
    set_status(racechrono_data_status_,
               !enabled ? "DATA: DISABLED" :
               connection_error ? "DATA: ERROR" :
               active ? "DATA: ACTIVE" : "DATA: WAITING",
               connection_error ? UiTheme::red() :
               active ? UiTheme::green() : UiTheme::muted());
    const char* gps_text = has_fix
        ? (status.gps_fix_type >= 3U ? "GPS: 3D FIX" : "GPS: 2D FIX")
        : "GPS: NO FIX";
    set_status(racechrono_gps_status_, gps_text,
               has_fix ? UiTheme::green() : UiTheme::muted());
    char text[64];
    if (racechrono_packets_) {
        std::snprintf(text, sizeof(text), "%u",
                      static_cast<unsigned>(enabled ? status.value_packets : 0U));
        lv_label_set_text(racechrono_packets_, text);
    }
    if (racechrono_active_) {
        std::snprintf(text, sizeof(text), "%u / %u",
                      static_cast<unsigned>(enabled ? status.active_channels : 0U),
            static_cast<unsigned>(status.configured_channels));
        lv_label_set_text(racechrono_active_, text);
    }
    if (racechrono_satellites_) {
        std::snprintf(text, sizeof(text), "%u",
                      static_cast<unsigned>(active ? status.satellites : 0U));
        lv_label_set_text(racechrono_satellites_, text);
    }
    if (racechrono_accuracy_) {
        if (has_fix)
            std::snprintf(text, sizeof(text), "%.1f m",
                          static_cast<double>(status.gps_accuracy));
        else
            std::snprintf(text, sizeof(text), "---");
        lv_label_set_text(racechrono_accuracy_, text);
    }

    for (std::size_t row = 0U;
         row < RaceChronoSettingsModel::kRowsPerPage; ++row) {
        if (!racechrono_channel_names_[row] ||
            !racechrono_channel_states_[row] ||
            !racechrono_channel_values_[row] ||
            row >= racechrono_settings_.rowsOnPage()) {
            if (racechrono_channel_names_[row])
                lv_obj_add_flag(racechrono_channel_names_[row],
                                LV_OBJ_FLAG_HIDDEN);
            if (racechrono_channel_states_[row])
                lv_obj_add_flag(racechrono_channel_states_[row],
                                LV_OBJ_FLAG_HIDDEN);
            if (racechrono_channel_values_[row])
                lv_obj_add_flag(racechrono_channel_values_[row],
                                LV_OBJ_FLAG_HIDDEN);
            continue;
        }
        lv_obj_clear_flag(racechrono_channel_names_[row], LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(racechrono_channel_states_[row], LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(racechrono_channel_values_[row], LV_OBJ_FLAG_HIDDEN);
        const std::size_t catalog_index =
            racechrono_settings_.catalogIndexAtRow(row);
        const ParameterId id = raceChronoChannelAt(catalog_index).parameter;
        lv_label_set_text(racechrono_channel_names_[row],
                          parameterDescriptor(id).name);
        const RaceChronoChannelState state =
            latest_racechrono_.channelState(id);
        lv_label_set_text(racechrono_channel_states_[row],
                          channelStateText(state));
        lv_obj_set_style_text_color(racechrono_channel_states_[row],
                                    channelStateColor(state), 0);
        const SignalValue& signal = latest_racechrono_.get(id);
        if (!signal.valid) {
            lv_label_set_text(racechrono_channel_values_[row], "---");
            continue;
        }
        const PresentedValue value = UnitPresenter::present(
            id, signal.value, settingsConfig().units);
        const unsigned decimals = parameterDescriptor(id).default_decimals;
        std::snprintf(text, sizeof(text), "%.*f %s",
                      static_cast<int>(decimals),
                      static_cast<double>(value.value), value.unit);
        lv_label_set_text(racechrono_channel_values_[row], text);
    }
    if (racechrono_page_number_) {
        std::snprintf(text, sizeof(text), "PAGE %u / %u",
            static_cast<unsigned>(racechrono_settings_.pageIndex() + 1U),
            static_cast<unsigned>(racechrono_settings_.pageCount()));
        lv_label_set_text(racechrono_page_number_, text);
    }
    if (racechrono_previous_page_) {
        if (racechrono_settings_.pageIndex() == 0U)
            lv_obj_add_state(racechrono_previous_page_, LV_STATE_DISABLED);
        else
            lv_obj_clear_state(racechrono_previous_page_, LV_STATE_DISABLED);
    }
    if (racechrono_next_page_) {
        if (racechrono_settings_.pageIndex() + 1U >=
            racechrono_settings_.pageCount())
            lv_obj_add_state(racechrono_next_page_, LV_STATE_DISABLED);
        else
            lv_obj_clear_state(racechrono_next_page_, LV_STATE_DISABLED);
    }
}

void Ui::raceChronoEvent(lv_event_t* event) {
    if (!instance_ || !instance_->config_) return;
    Ui* self = instance_;
    const intptr_t action = reinterpret_cast<intptr_t>(
        lv_event_get_user_data(event));
    if (action == ShowConnection || action == ShowChannels) {
        self->racechrono_channels_tab_ = action == ShowChannels;
        self->showSettings(SettingsCategory::RaceChrono);
        return;
    }
    if (action == ToggleEnabled) {
        self->setRaceChronoDraftEnabled(lv_obj_has_state(
            self->racechrono_enabled_, LV_STATE_CHECKED));
        return;
    }
    if (action == ToggleEnabledRow) {
        self->setRaceChronoDraftEnabled(
            !self->settingsConfig().racechrono.enabled);
        return;
    }
    if (action == RestartBle) {
        self->racechrono_restart_requested_ = true;
        return;
    }
    if (action == ChangeFilter) {
        self->racechrono_settings_.setFilter(
            static_cast<RaceChronoChannelFilter>(lv_dropdown_get_selected(
                lv_event_get_target(event))));
        self->showSettings(SettingsCategory::RaceChrono);
        return;
    }
    const bool changed = action == PreviousPage
        ? self->racechrono_settings_.previousPage()
        : self->racechrono_settings_.nextPage();
    if (changed) self->showSettings(SettingsCategory::RaceChrono);
}

void Ui::setRaceChronoDraftEnabled(bool enabled) {
    AppConfig candidate = settings_draft_;
    candidate.racechrono.enabled = enabled;
    if (!stageSettings(candidate, true)) return;
    if (racechrono_enabled_) {
        if (enabled)
            lv_obj_add_state(racechrono_enabled_, LV_STATE_CHECKED);
        else
            lv_obj_clear_state(racechrono_enabled_, LV_STATE_CHECKED);
    }
    refreshRaceChronoSettings();
}

void Ui::raceChronoBackEvent(lv_event_t*) {
    if (!instance_) return;
    instance_->requestSettingsExit(SettingsCategory::DataCan);
}

bool Ui::takeRaceChronoRestart() {
    const bool requested = racechrono_restart_requested_;
    racechrono_restart_requested_ = false;
    return requested;
}
