#include "ui.h"

#include <cstdio>

Ui* Ui::instance_ = nullptr;

namespace {
const lv_color_t kBg = lv_color_hex(0x07090D);
const lv_color_t kPanel = lv_color_hex(0x11161D);
const lv_color_t kBorder = lv_color_hex(0x27313C);
const lv_color_t kText = lv_color_hex(0xF2F5F7);
const lv_color_t kMuted = lv_color_hex(0x7D8996);
const lv_color_t kBlue = lv_color_hex(0x20A4F3);
const lv_color_t kGreen = lv_color_hex(0x39D353);
const lv_color_t kYellow = lv_color_hex(0xFFCF33);
const lv_color_t kRed = lv_color_hex(0xFF3B30);
const lv_color_t kPurple = lv_color_hex(0xB26BFF);
constexpr int kNavY = 420;
constexpr int kNavH = 60;

void styleScreen(lv_obj_t* screen) {
    lv_obj_set_style_bg_color(screen, kBg, 0);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, 0);
    lv_obj_set_style_text_color(screen, kText, 0);
    lv_obj_clear_flag(screen, LV_OBJ_FLAG_SCROLLABLE);
}

lv_obj_t* label(lv_obj_t* parent, const char* text, int x, int y,
                const lv_font_t* font, lv_color_t color = kText) {
    lv_obj_t* object = lv_label_create(parent);
    lv_label_set_text(object, text);
    lv_obj_set_pos(object, x, y);
    lv_obj_set_style_text_font(object, font, 0);
    lv_obj_set_style_text_color(object, color, 0);
    return object;
}

const char* canStatusText(CanStatus status) {
    switch (status) {
        case CanStatus::Waiting: return "WAITING";
        case CanStatus::Online: return "ONLINE";
        case CanStatus::Offline: return "OFFLINE";
        case CanStatus::InitFailed: return "INIT FAILED";
    }
    return "UNKNOWN";
}

const char* sourceText(DataSource source) {
    switch (source) {
        case DataSource::None: return "NO DATA";
        case DataSource::Can: return "CAN DATA";
        case DataSource::Demo: return "DEMO DATA";
    }
    return "NO DATA";
}
}

void Ui::begin() {
    instance_ = this;
    createDash();
    createTrack();
    createDiag();
    createSettings();
    load(Page::Dash);
}

lv_obj_t* Ui::createValueTile(lv_obj_t* parent, const char* title, int x,
                              int y, int width, int height,
                              lv_obj_t** value_label, lv_color_t accent) {
    lv_obj_t* tile = lv_obj_create(parent);
    lv_obj_set_pos(tile, x, y);
    lv_obj_set_size(tile, width, height);
    lv_obj_set_style_bg_color(tile, kPanel, 0);
    lv_obj_set_style_border_width(tile, 1, 0);
    lv_obj_set_style_border_color(tile, kBorder, 0);
    lv_obj_set_style_radius(tile, 8, 0);
    lv_obj_set_style_pad_all(tile, 8, 0);
    lv_obj_clear_flag(tile, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* stripe = lv_obj_create(tile);
    lv_obj_set_pos(stripe, 0, 0);
    lv_obj_set_size(stripe, 4, height - 2);
    lv_obj_set_style_bg_color(stripe, accent, 0);
    lv_obj_set_style_border_width(stripe, 0, 0);
    lv_obj_set_style_radius(stripe, 2, 0);
    lv_obj_clear_flag(stripe, LV_OBJ_FLAG_SCROLLABLE);

    label(tile, title, 10, 2, &lv_font_montserrat_12, kMuted);
    *value_label = label(tile, "---", 10, 23, &lv_font_montserrat_24);
    return tile;
}

void Ui::createShiftBar(lv_obj_t* parent, lv_obj_t** segments) {
    for (int i = 0; i < 12; ++i) {
        segments[i] = lv_obj_create(parent);
        lv_obj_set_pos(segments[i], 14 + i * 64, 10);
        lv_obj_set_size(segments[i], 52, 16);
        lv_obj_set_style_border_width(segments[i], 0, 0);
        lv_obj_set_style_radius(segments[i], 4, 0);
        lv_obj_set_style_bg_color(segments[i], lv_color_hex(0x202830), 0);
        lv_obj_clear_flag(segments[i], LV_OBJ_FLAG_SCROLLABLE);
    }
}

void Ui::createNavigation(lv_obj_t* parent, Page active) {
    const char* names[] = {"DASH", "TRACK", "DIAG", "SETTINGS"};
    for (int i = 0; i < 4; ++i) {
        lv_obj_t* button = lv_btn_create(parent);
        lv_obj_set_pos(button, i * 200, kNavY);
        lv_obj_set_size(button, 200, kNavH);
        lv_obj_set_style_radius(button, 0, 0);
        lv_obj_set_style_bg_color(
            button, static_cast<int>(active) == i ? lv_color_hex(0x153B57)
                                                  : lv_color_hex(0x10151B), 0);
        lv_obj_set_style_border_width(button, 0, 0);
        lv_obj_add_event_cb(button, navEvent, LV_EVENT_CLICKED,
                            reinterpret_cast<void*>(static_cast<intptr_t>(i)));
        lv_obj_t* text = lv_label_create(button);
        lv_label_set_text(text, names[i]);
        lv_obj_set_style_text_font(text, &lv_font_montserrat_14, 0);
        lv_obj_center(text);
    }
}

void Ui::createDash() {
    dash_ = lv_obj_create(nullptr);
    styleScreen(dash_);
    createShiftBar(dash_, dash_shift_);

    label(dash_, "RPM", 332, 42, &lv_font_montserrat_14, kMuted);
    dash_rpm_ = label(dash_, "---", 294, 58, &lv_font_montserrat_48);
    label(dash_, "GEAR", 610, 42, &lv_font_montserrat_14, kMuted);
    dash_gear_ = label(dash_, "-", 626, 56, &lv_font_montserrat_48, kYellow);

    createValueTile(dash_, "SPEED km/h", 12, 45, 180, 78, &dash_speed_, kBlue);
    createValueTile(dash_, "MAP / BOOST bar", 12, 130, 180, 78, &dash_map_, kBlue);
    dash_lambda_tile_ = createValueTile(dash_, "LAMBDA", 12, 215, 180, 78,
                                        &dash_lambda_, kPurple);
    dash_clt_tile_ = createValueTile(dash_, "CLT C", 12, 300, 180, 78,
                                     &dash_clt_, kBlue);

    dash_iat_tile_ = createValueTile(dash_, "IAT C", 608, 130, 180, 78,
                                     &dash_iat_, kBlue);
    dash_oil_p_tile_ = createValueTile(dash_, "OIL PRESS bar", 608, 215,
                                       180, 78, &dash_oil_p_, kYellow);
    createValueTile(dash_, "OIL TEMP C", 608, 300, 180, 78, &dash_oil_t_, kYellow);

    createValueTile(dash_, "FUEL PRESS bar", 210, 300, 180, 78,
                    &dash_fuel_p_, kGreen);
    dash_batt_tile_ = createValueTile(dash_, "BATTERY V", 405, 300, 180, 78,
                                      &dash_batt_, kRed);
    createValueTile(dash_, "TPS %", 210, 210, 180, 78, &dash_tps_, kGreen);
    createValueTile(dash_, "CAN / SOURCE", 405, 210, 180, 78, &dash_can_, kBlue);
    createNavigation(dash_, Page::Dash);
}

void Ui::createTrack() {
    track_ = lv_obj_create(nullptr);
    styleScreen(track_);
    createShiftBar(track_, track_shift_);

    label(track_, "RPM", 324, 46, &lv_font_montserrat_14, kMuted);
    track_rpm_ = label(track_, "---", 285, 62, &lv_font_montserrat_48);
    label(track_, "GEAR", 612, 46, &lv_font_montserrat_14, kMuted);
    track_gear_ = label(track_, "-", 628, 60, &lv_font_montserrat_48, kYellow);

    createValueTile(track_, "SPEED km/h", 18, 70, 180, 80, &track_speed_, kBlue);
    createValueTile(track_, "MAP / BOOST bar", 18, 158, 180, 80, &track_map_, kBlue);
    createValueTile(track_, "LAMBDA", 18, 246, 180, 80, &track_lambda_, kPurple);
    createValueTile(track_, "OIL PRESS bar", 602, 158, 180, 80,
                    &track_oil_p_, kYellow);
    createValueTile(track_, "CLT C", 602, 246, 180, 80, &track_clt_, kBlue);

    label(track_, "TRACK VIEW", 315, 190, &lv_font_montserrat_24, kMuted);
    label(track_, "Lap timing module ready", 285, 230, &lv_font_montserrat_14,
          kMuted);
    createNavigation(track_, Page::Track);
}

void Ui::createDiag() {
    diag_ = lv_obj_create(nullptr);
    styleScreen(diag_);
    label(diag_, "DIY DASH DIAGNOSTICS", 22, 18, &lv_font_montserrat_28);
    label(diag_, "WAVESHARE ESP32-S3-TOUCH-LCD-5 / 800x480", 24, 56,
          &lv_font_montserrat_14, kMuted);

    diag_status_ = label(diag_, "Display: ---\nTouch: ---\nResolution: 800x480",
                         30, 105, &lv_font_montserrat_20);
    diag_memory_ = label(diag_, "PSRAM: ---\nFree heap: ---", 315, 105,
                         &lv_font_montserrat_20);
    diag_runtime_ = label(diag_, "Uptime: ---\nUI updates: ---", 570, 105,
                          &lv_font_montserrat_20);

    lv_obj_t* can_box = lv_obj_create(diag_);
    lv_obj_set_pos(can_box, 30, 255);
    lv_obj_set_size(can_box, 740, 115);
    lv_obj_set_style_bg_color(can_box, kPanel, 0);
    lv_obj_set_style_border_color(can_box, kBlue, 0);
    lv_obj_set_style_border_width(can_box, 2, 0);
    lv_obj_set_style_radius(can_box, 8, 0);
    label(can_box, "CAN / DECODER", 20, 10, &lv_font_montserrat_16, kMuted);
    diag_can_ = label(can_box, "---", 20, 40, &lv_font_montserrat_20);
    createNavigation(diag_, Page::Diag);
}

void Ui::createSettings() {
    settings_ = lv_obj_create(nullptr);
    styleScreen(settings_);
    label(settings_, "SETTINGS", 24, 22, &lv_font_montserrat_28);
    label(settings_, "Read-only baseline configuration", 24, 62,
          &lv_font_montserrat_14, kMuted);
    lv_obj_t* panel = lv_obj_create(settings_);
    lv_obj_set_pos(panel, 24, 105);
    lv_obj_set_size(panel, 752, 260);
    lv_obj_set_style_bg_color(panel, kPanel, 0);
    lv_obj_set_style_border_color(panel, kBorder, 0);
    lv_obj_set_style_border_width(panel, 1, 0);
    settings_can_ = label(panel, "CAN bitrate: ---\nTimeout: ---\nDEMO: ---\nDecoder mappings: ---",
                          20, 20, &lv_font_montserrat_20);
    label(panel, "Validated ECU profiles can be added without changing the UI.",
          20, 185, &lv_font_montserrat_14, kMuted);
    createNavigation(settings_, Page::Settings);
}

void Ui::updateValue(lv_obj_t* value_label, const VehicleState& state,
                     VehicleSignal signal, const char* format) {
    const SignalValue& value = state.get(signal);
    if (!value.valid) {
        lv_label_set_text(value_label, "---");
        return;
    }
    lv_label_set_text_fmt(value_label, format, static_cast<double>(value.value));
}

void Ui::styleAlarm(lv_obj_t* tile, bool active, AlarmSeverity severity) {
    lv_obj_set_style_border_width(tile, active ? 3 : 1, 0);
    lv_obj_set_style_border_color(
        tile, active ? (severity == AlarmSeverity::Critical ? kRed : kYellow)
                     : kBorder,
        0);
}

void Ui::updateShiftBar(lv_obj_t** segments, uint16_t rpm) {
    const int lit = static_cast<int>((static_cast<uint32_t>(rpm) * 12U) / 8000U);
    for (int i = 0; i < 12; ++i) {
        lv_color_t color = lv_color_hex(0x202830);
        if (i < lit) {
            color = i < 6 ? kGreen : (i < 9 ? kYellow : kRed);
        }
        lv_obj_set_style_bg_color(segments[i], color, 0);
    }
}

void Ui::update(const VehicleState& state, const RuntimeDiagnostics& diagnostics,
                const UiRuntimeStatus& status) {
    char buffer[192];

    updateValue(dash_rpm_, state, VehicleSignal::Rpm, "%.0f");
    updateValue(dash_gear_, state, VehicleSignal::Gear, "%.0f");
    updateValue(dash_speed_, state, VehicleSignal::Speed, "%.0f");
    updateValue(dash_map_, state, VehicleSignal::Map, "%.2f");
    updateValue(dash_lambda_, state, VehicleSignal::Lambda, "%.2f");
    updateValue(dash_clt_, state, VehicleSignal::Clt, "%.0f");
    updateValue(dash_iat_, state, VehicleSignal::Iat, "%.0f");
    updateValue(dash_oil_p_, state, VehicleSignal::OilPressure, "%.1f");
    updateValue(dash_oil_t_, state, VehicleSignal::OilTemperature, "%.0f");
    updateValue(dash_fuel_p_, state, VehicleSignal::FuelPressure, "%.1f");
    updateValue(dash_batt_, state, VehicleSignal::BatteryVoltage, "%.2f");
    updateValue(dash_tps_, state, VehicleSignal::Tps, "%.0f");
    std::snprintf(buffer, sizeof(buffer), "%s\n%s", canStatusText(status.can_status),
                  sourceText(state.source()));
    lv_label_set_text(dash_can_, buffer);
    lv_obj_set_style_text_color(dash_can_, status.demo_active ? kYellow : kText, 0);

    const SignalValue& rpm = state.get(VehicleSignal::Rpm);
    updateShiftBar(dash_shift_, rpm.valid ? static_cast<uint16_t>(rpm.value) : 0U);
    styleAlarm(dash_clt_tile_, status.alarms.active(AlarmId::HighClt),
               status.alarms.severityFor(AlarmId::HighClt));
    styleAlarm(dash_iat_tile_, status.alarms.active(AlarmId::HighIat),
               status.alarms.severityFor(AlarmId::HighIat));
    styleAlarm(dash_oil_p_tile_, status.alarms.active(AlarmId::LowOilPressure),
               status.alarms.severityFor(AlarmId::LowOilPressure));
    styleAlarm(dash_lambda_tile_, status.alarms.active(AlarmId::LeanLambda),
               status.alarms.severityFor(AlarmId::LeanLambda));
    styleAlarm(dash_batt_tile_, status.alarms.active(AlarmId::LowBattery),
               status.alarms.severityFor(AlarmId::LowBattery));

    std::snprintf(buffer, sizeof(buffer), "Display: %s\nTouch: %s\nResolution: 800x480",
                  diagnostics.display_ok ? "OK" : "FAIL",
                  diagnostics.touch_ok ? "OK" : "NO");
    lv_label_set_text(diag_status_, buffer);
    std::snprintf(buffer, sizeof(buffer), "PSRAM: %.1f MB\nFree heap: %u KB",
                  static_cast<double>(diagnostics.psram_total) / (1024.0 * 1024.0),
                  static_cast<unsigned>(diagnostics.free_heap / 1024U));
    lv_label_set_text(diag_memory_, buffer);
    std::snprintf(buffer, sizeof(buffer), "Uptime: %u s\nUI updates: %u",
                  static_cast<unsigned>(diagnostics.uptime_ms / 1000U),
                  static_cast<unsigned>(diagnostics.ui_updates));
    lv_label_set_text(diag_runtime_, buffer);
    std::snprintf(buffer, sizeof(buffer),
                  "Status: %s / %s\nRX frames: %u   rejected: %u   mappings: %u",
                  canStatusText(status.can_status), sourceText(state.source()),
                  static_cast<unsigned>(status.received_frames),
                  static_cast<unsigned>(status.rejected_frames),
                  static_cast<unsigned>(status.decoder_mappings));
    lv_label_set_text(diag_can_, buffer);

    updateValue(track_rpm_, state, VehicleSignal::Rpm, "%.0f");
    updateValue(track_gear_, state, VehicleSignal::Gear, "%.0f");
    updateValue(track_speed_, state, VehicleSignal::Speed, "%.0f");
    updateValue(track_map_, state, VehicleSignal::Map, "%.2f");
    updateValue(track_lambda_, state, VehicleSignal::Lambda, "%.2f");
    updateValue(track_oil_p_, state, VehicleSignal::OilPressure, "%.1f");
    updateValue(track_clt_, state, VehicleSignal::Clt, "%.0f");
    updateShiftBar(track_shift_, rpm.valid ? static_cast<uint16_t>(rpm.value) : 0U);

    std::snprintf(buffer, sizeof(buffer),
                  "CAN bitrate: %u kbit/s\nTimeout: %u ms\nDEMO: %s\nDecoder mappings: %u",
                  static_cast<unsigned>(status.can_bitrate / 1000U),
                  static_cast<unsigned>(status.can_timeout_ms),
                  status.demo_active ? "ACTIVE" : "STANDBY",
                  static_cast<unsigned>(status.decoder_mappings));
    lv_label_set_text(settings_can_, buffer);
}

void Ui::navEvent(lv_event_t* event) {
    if (instance_ == nullptr) {
        return;
    }
    const auto page = static_cast<Page>(
        reinterpret_cast<intptr_t>(lv_event_get_user_data(event)));
    instance_->load(page);
}

void Ui::load(Page page) {
    current_page_ = page;
    switch (page) {
        case Page::Dash: lv_scr_load(dash_); break;
        case Page::Track: lv_scr_load(track_); break;
        case Page::Diag: lv_scr_load(diag_); break;
        case Page::Settings: lv_scr_load(settings_); break;
    }
}
