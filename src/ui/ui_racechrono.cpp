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
    RestartBle,
    ChangeFilter,
    PreviousPage,
    NextPage,
};

const char* connectionText(RaceChronoConnectionState state) {
    switch (state) {
        case RaceChronoConnectionState::Disabled: return "DISABLED";
        case RaceChronoConnectionState::Advertising: return "ADVERTISING";
        case RaceChronoConnectionState::Connected: return "CONNECTED";
        case RaceChronoConnectionState::Configuring: return "CONFIGURING";
        case RaceChronoConnectionState::Active: return "ACTIVE";
        case RaceChronoConnectionState::NoData: return "NO DATA";
        case RaceChronoConnectionState::Error: return "ERROR";
    }
    return "UNKNOWN";
}

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
        constexpr const char* names[] = {
            "ENABLED", "CONNECTION", "BLE DEVICE", "LAST DATA"};
        for (int row = 0; row < 4; ++row) {
            clippedLabel(content, names[row], 16, row * 42 + 14, 220,
                         UiTheme::muted());
            addSeparator(content, (row + 1) * 42 - 1, 750);
        }

        racechrono_enabled_ = lv_checkbox_create(content);
        lv_obj_set_pos(racechrono_enabled_, 684, 9);
        lv_checkbox_set_text(racechrono_enabled_, "");
        darkCheckbox(racechrono_enabled_);
        if (config_->racechrono.enabled)
            lv_obj_add_state(racechrono_enabled_, LV_STATE_CHECKED);
        lv_obj_add_event_cb(racechrono_enabled_, raceChronoEvent,
                            LV_EVENT_VALUE_CHANGED,
                            reinterpret_cast<void*>(ToggleEnabled));

        racechrono_connection_ = clippedLabel(
            content, "", 430, 56, 286, UiTheme::green());
        lv_obj_set_style_text_align(racechrono_connection_,
                                    LV_TEXT_ALIGN_RIGHT, 0);
        clippedLabel(content, "DIY DASH RC", 430, 98, 286);
        racechrono_last_data_ = clippedLabel(content, "", 430, 140, 286);
        lv_obj_set_style_text_align(racechrono_last_data_,
                                    LV_TEXT_ALIGN_RIGHT, 0);

        constexpr const char* summary_names[] = {
            "BLE PACKETS", "ACTIVE CHANNELS", "SATELLITES", "GPS ACCURACY"};
        lv_obj_t** summary_values[] = {
            &racechrono_packets_, &racechrono_active_,
            &racechrono_satellites_, &racechrono_accuracy_};
        for (int index = 0; index < 4; ++index) {
            const int column = index % 2;
            const int row = index / 2;
            const int x = 16 + column * 370;
            clippedLabel(content, summary_names[index], x,
                         177 + row * 26, 170, UiTheme::muted());
            *summary_values[index] = clippedLabel(
                content, "", x + 174, 177 + row * 26, 160);
            lv_obj_set_style_text_align(*summary_values[index],
                                        LV_TEXT_ALIGN_RIGHT, 0);
        }
        addSeparator(content, 227, 750);
        racechrono_signal_ = clippedLabel(content, "", 16, 240, 480,
                                           UiTheme::green());
        button(content, "RESTART BLE", 568, 237, 166, 38,
               raceChronoEvent, RestartBle);
    } else {
        clippedLabel(content, "CHANNEL FILTER", 16, 14, 220,
                     UiTheme::muted());
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

        const std::size_t rows = racechrono_settings_.rowsOnPage();
        for (std::size_t row = 0U;
             row < RaceChronoSettingsModel::kRowsPerPage; ++row) {
            const int y = 42 + static_cast<int>(row) * 32;
            if (row < rows) {
                const std::size_t catalog_index =
                    racechrono_settings_.catalogIndexAtRow(row);
                const ParameterId id =
                    raceChronoChannelAt(catalog_index).parameter;
                clippedLabel(content, parameterDescriptor(id).name,
                             16, y + 10, 330);
                racechrono_channel_states_[row] = clippedLabel(
                    content, "", 360, y + 10, 130, UiTheme::muted());
                racechrono_channel_values_[row] = clippedLabel(
                    content, "", 500, y + 10, 218);
                lv_obj_set_style_text_align(racechrono_channel_values_[row],
                                            LV_TEXT_ALIGN_RIGHT, 0);
            }
            addSeparator(content, y + 31, 750);
        }

        lv_obj_t* previous = button(content, "< PREVIOUS", 16, 243, 150, 34,
                                    raceChronoEvent, PreviousPage);
        if (racechrono_settings_.pageIndex() == 0U)
            lv_obj_add_state(previous, LV_STATE_DISABLED);
        char page_text[32];
        std::snprintf(page_text, sizeof(page_text), "PAGE %u / %u",
            static_cast<unsigned>(racechrono_settings_.pageIndex() + 1U),
            static_cast<unsigned>(racechrono_settings_.pageCount()));
        lv_obj_t* page_number = clippedLabel(content, page_text, 276, 254,
                                              200, UiTheme::muted());
        lv_obj_set_style_text_align(page_number, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_t* next = button(content, "NEXT >", 584, 243, 150, 34,
                                raceChronoEvent, NextPage);
        if (racechrono_settings_.pageIndex() + 1U >=
            racechrono_settings_.pageCount())
            lv_obj_add_state(next, LV_STATE_DISABLED);
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
    if (racechrono_connection_) {
        lv_label_set_text(racechrono_connection_,
                          connectionText(latest_ui_status_.racechrono_connection));
        lv_obj_set_style_text_color(
            racechrono_connection_,
            latest_ui_status_.racechrono_connection ==
                    RaceChronoConnectionState::Active
                ? UiTheme::green()
                : latest_ui_status_.racechrono_connection ==
                          RaceChronoConnectionState::Error
                    ? UiTheme::red() : UiTheme::muted(), 0);
    }
    char text[64];
    if (racechrono_last_data_) {
        if (status.has_valid_packet)
            std::snprintf(text, sizeof(text), "%u ms",
                          static_cast<unsigned>(status.last_packet_age_ms));
        else
            std::snprintf(text, sizeof(text), "---");
        lv_label_set_text(racechrono_last_data_, text);
    }
    if (racechrono_packets_) {
        std::snprintf(text, sizeof(text), "%u",
                      static_cast<unsigned>(status.value_packets));
        lv_label_set_text(racechrono_packets_, text);
    }
    if (racechrono_active_) {
        std::snprintf(text, sizeof(text), "%u / %u",
            static_cast<unsigned>(status.active_channels),
            static_cast<unsigned>(status.configured_channels));
        lv_label_set_text(racechrono_active_, text);
    }
    if (racechrono_satellites_) {
        std::snprintf(text, sizeof(text), "%u",
                      static_cast<unsigned>(status.satellites));
        lv_label_set_text(racechrono_satellites_, text);
    }
    if (racechrono_accuracy_) {
        std::snprintf(text, sizeof(text), "%.1f m",
                      static_cast<double>(status.gps_accuracy));
        lv_label_set_text(racechrono_accuracy_, text);
    }
    if (racechrono_signal_) {
        std::snprintf(text, sizeof(text), "%s · %s FIX",
            status.has_valid_packet ? "GOOD" : "WAITING",
            status.gps_fix_type >= 3U ? "3D" :
            status.gps_fix_type == 2U ? "2D" : "NO");
        lv_label_set_text(racechrono_signal_, text);
    }

    for (std::size_t row = 0U;
         row < RaceChronoSettingsModel::kRowsPerPage; ++row) {
        if (!racechrono_channel_states_[row] ||
            !racechrono_channel_values_[row] ||
            row >= racechrono_settings_.rowsOnPage()) continue;
        const std::size_t catalog_index =
            racechrono_settings_.catalogIndexAtRow(row);
        const ParameterId id = raceChronoChannelAt(catalog_index).parameter;
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
            id, signal.value, config_->units);
        const unsigned decimals = parameterDescriptor(id).default_decimals;
        std::snprintf(text, sizeof(text), "%.*f %s",
                      static_cast<int>(decimals),
                      static_cast<double>(value.value), value.unit);
        lv_label_set_text(racechrono_channel_values_[row], text);
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
        AppConfig candidate = *self->config_;
        candidate.racechrono.enabled = lv_obj_has_state(
            self->racechrono_enabled_, LV_STATE_CHECKED);
        self->stageSettings(candidate, true);
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

void Ui::raceChronoBackEvent(lv_event_t*) {
    if (!instance_) return;
    instance_->queueSettingsOnExit();
    instance_->showSettings(SettingsCategory::DataCan);
}

bool Ui::takeRaceChronoRestart() {
    const bool requested = racechrono_restart_requested_;
    racechrono_restart_requested_ = false;
    return requested;
}
