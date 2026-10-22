#include "ui.h"

#include <cinttypes>
#include <cmath>
#include <cstdio>
#include <cstring>

#include "ui/editor_widgets.h"
#include "ui/unit_presenter.h"

using namespace EditorWidgets;

namespace {
enum OdometerAction : intptr_t {
    kOdoSet = 1,
    kOdoSourceCan,
    kOdoSourceWheels,
    kOdoSourceGps,
    kOdoFallback,
    kOdoResetTripA,
    kOdoResetTripB,
    kOdoCorrectionDown,
    kOdoCorrectionUp,
};

constexpr int16_t kCorrectionStepPermille = 1;  // 0.1 %

lv_obj_t* card(lv_obj_t* parent, int x, int y, int w, int h, const char* title) {
    lv_obj_t* o = lv_obj_create(parent);
    lv_obj_set_pos(o, x, y);
    lv_obj_set_size(o, w, h);
    lv_obj_set_style_bg_color(o, UiTheme::panel(), 0);
    lv_obj_set_style_border_color(o, UiTheme::border(), 0);
    lv_obj_set_style_border_width(o, 1, 0);
    lv_obj_set_style_radius(o, 10, 0);
    lv_obj_set_style_pad_all(o, 0, 0);
    lv_obj_clear_flag(o, LV_OBJ_FLAG_SCROLLABLE);
    label(o, title, 16, 12, &lv_font_montserrat_12, UiTheme::muted());
    return o;
}

void setText(lv_obj_t* object, const char* text) {
    if (object && std::strcmp(lv_label_get_text(object), text) != 0)
        lv_label_set_text(object, text);
}

void markSelected(lv_obj_t* button, bool selected) {
    if (!button) return;
    lv_obj_set_style_border_color(button, selected ? UiTheme::blue() : UiTheme::border(), 0);
    lv_obj_set_style_border_width(button, selected ? 2 : 1, 0);
}
}  // namespace

void Ui::createOdometerSettings(lv_obj_t* panel) {
    if (!odometer_) {
        label(panel, "Odometer unavailable", 24, 24, &lv_font_montserrat_20);
        return;
    }
    const PresentedValue unit = UnitPresenter::present(
        ParameterId::Odometer, 0.0f, settingsConfig().units);

    lv_obj_t* odo = card(panel, 0, 0, 468, 150, "ODOMETER");
    odometer_value_ = label(odo, "", 16, 34, &lv_font_montserrat_48);
    odometer_status_ = label(odo, "", 16, 116, &lv_font_montserrat_12, UiTheme::muted());
    button(odo, "SET VALUE", 312, 90, 140, 44, odometerEvent, kOdoSet, true);
    // Hidden spinbox carries the keypad binding (0.1 km resolution).
    odometer_field_ = lv_spinbox_create(odo);
    lv_spinbox_set_range(odometer_field_, 0, 9999999);
    lv_spinbox_set_digit_format(odometer_field_, 7, 6);
    lv_obj_add_flag(odometer_field_, LV_OBJ_FLAG_HIDDEN);
    odometer_binding_index_ = numeric_binding_count_;
    bindNumeric(odometer_field_, "SET ODOMETER", 0.0f, 999999.9f, 1, unit.unit);

    lv_obj_t* source = card(panel, 480, 0, 288, 150, "DISTANCE SOURCE");
    const char* names[] = {"CAN SPEED", "WHEELS", "GPS"};
    for (int i = 0; i < 3; ++i) {
        odometer_source_[i] = button(source, names[i], 12 + i * 90, 38, 84, 44,
                                     odometerEvent, kOdoSourceCan + i);
        lv_obj_set_style_text_font(lv_obj_get_child(odometer_source_[i], 0),
                                   &lv_font_montserrat_12, 0);
    }
    odometer_fallback_ = lv_checkbox_create(source);
    lv_checkbox_set_text(odometer_fallback_, "GPS fallback when CAN is lost");
    lv_obj_set_pos(odometer_fallback_, 14, 100);
    darkCheckbox(odometer_fallback_);
    lv_obj_add_event_cb(odometer_fallback_, odometerEvent, LV_EVENT_VALUE_CHANGED,
                        reinterpret_cast<void*>(kOdoFallback));

    const char* trip_titles[] = {"TRIP A", "TRIP B"};
    for (int i = 0; i < 2; ++i) {
        lv_obj_t* trip = card(panel, i * 392, 162, 376, 110, trip_titles[i]);
        trip_value_[i] = label(trip, "", 16, 32, &lv_font_montserrat_40);
        trip_detail_[i] = label(trip, "", 16, 84, &lv_font_montserrat_12, UiTheme::muted());
        lv_obj_t* reset = button(trip, "HOLD TO RESET", 224, 30, 136, 48, nullptr);
        lv_obj_set_style_text_font(lv_obj_get_child(reset, 0), &lv_font_montserrat_12, 0);
        lv_obj_add_event_cb(reset, odometerEvent, LV_EVENT_LONG_PRESSED,
                            reinterpret_cast<void*>(i == 0 ? kOdoResetTripA : kOdoResetTripB));
    }

    lv_obj_t* correction = card(panel, 0, 284, 768, 64, "");
    label(correction, "SPEED CORRECTION", 16, 25, &lv_font_montserrat_12, UiTheme::muted());
    button(correction, "-", 170, 10, 56, 44, odometerEvent, kOdoCorrectionDown);
    odometer_correction_ = label(correction, "", 236, 20, &lv_font_montserrat_24);
    lv_obj_set_width(odometer_correction_, 110);
    lv_obj_set_style_text_align(odometer_correction_, LV_TEXT_ALIGN_CENTER, 0);
    button(correction, "+", 356, 10, 56, 44, odometerEvent, kOdoCorrectionUp);
    lv_obj_t* hint = label(correction,
        "Applies to ODO and both trips. Calibrate against GPS over 10 km or more.",
        428, 16, &lv_font_montserrat_12, UiTheme::muted());
    lv_obj_set_width(hint, 330);
    lv_label_set_long_mode(hint, LV_LABEL_LONG_WRAP);

    refreshOdometerSettings();
}

void Ui::refreshOdometerSettings() {
    if (!odometer_ || !odometer_value_) return;
    const UnitSettings& units = settingsConfig().units;
    const PresentedValue odo = UnitPresenter::present(
        ParameterId::Odometer, static_cast<float>(odometer_->odometerKm()), units);
    char text[96];
    std::snprintf(text, sizeof(text), "%.1f %s", static_cast<double>(odo.value), odo.unit);
    setText(odometer_value_, text);
    if (odometer_field_)
        lv_spinbox_set_value(odometer_field_,
                             static_cast<int32_t>(std::lround(odo.value * 10.0f)));
    if (odometer_->storageError()) {
        setText(odometer_status_, "STORAGE ERROR - retrying");
        lv_obj_set_style_text_color(odometer_status_, UiTheme::red(), 0);
    } else {
        const PresentedValue pending = UnitPresenter::present(
            ParameterId::Odometer,
            static_cast<float>(odometer_->unsavedMeters() / 1000.0), units);
        std::snprintf(text, sizeof(text),
                      "Saved at every stop and every 1 km | unsaved %.2f %s",
                      static_cast<double>(pending.value), pending.unit);
        setText(odometer_status_, text);
        lv_obj_set_style_text_color(odometer_status_, UiTheme::muted(), 0);
    }

    const OdometerSettings& settings = odometer_->settings();
    for (int i = 0; i < 3; ++i)
        markSelected(odometer_source_[i], static_cast<int>(settings.source) == i);
    if (odometer_fallback_) {
        if (settings.gps_fallback) lv_obj_add_state(odometer_fallback_, LV_STATE_CHECKED);
        else lv_obj_clear_state(odometer_fallback_, LV_STATE_CHECKED);
        if (settings.source == DistanceSource::Gps)
            lv_obj_add_state(odometer_fallback_, LV_STATE_DISABLED);
        else
            lv_obj_clear_state(odometer_fallback_, LV_STATE_DISABLED);
    }

    const TripId trips[] = {TripId::A, TripId::B};
    for (int i = 0; i < 2; ++i) {
        const PresentedValue trip = UnitPresenter::present(
            ParameterId::TripA, static_cast<float>(odometer_->tripKm(trips[i])), units);
        std::snprintf(text, sizeof(text), "%.1f %s", static_cast<double>(trip.value), trip.unit);
        setText(trip_value_[i], text);
        const PresentedValue average = UnitPresenter::present(
            ParameterId::Speed, odometer_->tripAverageKph(trips[i]), units);
        const uint64_t minutes = odometer_->tripMovingMs(trips[i]) / 60000U;
        std::snprintf(text, sizeof(text), "avg %.0f %s | %" PRIu64 ":%02u h moving",
                      static_cast<double>(average.value), average.unit,
                      minutes / 60U, static_cast<unsigned>(minutes % 60U));
        setText(trip_detail_[i], text);
    }

    std::snprintf(text, sizeof(text), "%+.1f %%",
                  settings.correction_permille / 10.0);
    setText(odometer_correction_, text);
}

void Ui::applyOdometerField() {
    if (!odometer_ || !odometer_field_) return;
    const float presented = lv_spinbox_get_value(odometer_field_) / 10.0f;
    const float km = UnitPresenter::toNative(ParameterId::Odometer, presented,
                                             settingsConfig().units);
    if (!odometer_->setOdometerKm(km, lv_tick_get()))
        showSettingsMessage("INVALID VALUE");
    refreshOdometerSettings();
}

void Ui::odometerEvent(lv_event_t* event) {
    if (!instance_ || !instance_->odometer_) return;
    Ui* self = instance_;
    OdometerModel& odometer = *self->odometer_;
    const intptr_t action = reinterpret_cast<intptr_t>(lv_event_get_user_data(event));
    OdometerSettings settings = odometer.settings();
    switch (action) {
        case kOdoSet:
            self->openNumericEntry(self->odometer_binding_index_);
            return;
        case kOdoSourceCan:
        case kOdoSourceWheels:
        case kOdoSourceGps:
            settings.source = static_cast<DistanceSource>(action - kOdoSourceCan);
            odometer.setSettings(settings);
            break;
        case kOdoFallback:
            settings.gps_fallback =
                lv_obj_has_state(self->odometer_fallback_, LV_STATE_CHECKED);
            odometer.setSettings(settings);
            break;
        case kOdoResetTripA:
            odometer.resetTrip(TripId::A, lv_tick_get());
            break;
        case kOdoResetTripB:
            odometer.resetTrip(TripId::B, lv_tick_get());
            break;
        case kOdoCorrectionDown:
        case kOdoCorrectionUp: {
            const int step = action == kOdoCorrectionUp ? kCorrectionStepPermille
                                                        : -kCorrectionStepPermille;
            const int next = settings.correction_permille + step;
            if (next < -OdometerModel::kMaximumCorrectionPermille ||
                next > OdometerModel::kMaximumCorrectionPermille) return;
            settings.correction_permille = static_cast<int16_t>(next);
            odometer.setSettings(settings);
            break;
        }
        default:
            return;
    }
    self->refreshOdometerSettings();
}
