#include "ui.h"

#include <cstdio>

#include "ui/settings_screen_model.h"

Ui* Ui::instance_ = nullptr;

namespace {
const lv_color_t kBg = lv_color_hex(0x07090D);
const lv_color_t kPanel = lv_color_hex(0x11161D);
const lv_color_t kText = lv_color_hex(0xF2F5F7);
const lv_color_t kMuted = lv_color_hex(0x7D8996);
const lv_color_t kBlue = lv_color_hex(0x20A4F3);
const lv_color_t kGreen = lv_color_hex(0x39D353);
const lv_color_t kYellow = lv_color_hex(0xFFCF33);
const lv_color_t kRed = lv_color_hex(0xFF3B30);
const lv_color_t kPurple = lv_color_hex(0xB26BFF);

const char* dataSourceName(DataSource source) {
    switch (source) {
        case DataSource::Mock: return "MOCK";
        case DataSource::Ecumaster: return "ECUMASTER";
        case DataSource::Rusefi: return "RUSEFI";
    }
    return "UNKNOWN";
}

void styleScreen(lv_obj_t* screen) {
    lv_obj_set_style_bg_color(screen, kBg, 0);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, 0);
    lv_obj_set_style_text_color(screen, kText, 0);
    lv_obj_clear_flag(screen, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(screen, LV_OBJ_FLAG_GESTURE_BUBBLE);
}

lv_obj_t* label(lv_obj_t* parent, const char* text, int x, int y, const lv_font_t* font, lv_color_t color = kText) {
    lv_obj_t* obj = lv_label_create(parent);
    lv_label_set_text(obj, text);
    lv_obj_set_pos(obj, x, y);
    lv_obj_set_style_text_font(obj, font, 0);
    lv_obj_set_style_text_color(obj, color, 0);
    lv_obj_add_flag(obj, LV_OBJ_FLAG_GESTURE_BUBBLE);
    return obj;
}

GestureDirection gestureFromLvgl(lv_dir_t dir) {
    switch (dir) {
        case LV_DIR_LEFT: return GestureDirection::Left;
        case LV_DIR_RIGHT: return GestureDirection::Right;
        case LV_DIR_TOP: return GestureDirection::Up;
        case LV_DIR_BOTTOM: return GestureDirection::Down;
        default: return GestureDirection::None;
    }
}
}

void Ui::begin() {
    instance_ = this;
    createDash();
    createTrack();
    createDiag();
    createSettings();
    load(UiPage::Dash);
}

lv_obj_t* Ui::createValueTile(lv_obj_t* parent, const char* title, int x, int y, int w, int h,
                              lv_obj_t** value_label, lv_color_t accent) {
    lv_obj_t* tile = lv_obj_create(parent);
    lv_obj_set_pos(tile, x, y);
    lv_obj_set_size(tile, w, h);
    lv_obj_set_style_bg_color(tile, kPanel, 0);
    lv_obj_set_style_border_width(tile, 1, 0);
    lv_obj_set_style_border_color(tile, lv_color_hex(0x27313C), 0);
    lv_obj_set_style_radius(tile, 8, 0);
    lv_obj_set_style_pad_all(tile, 8, 0);
    lv_obj_clear_flag(tile, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(tile, LV_OBJ_FLAG_GESTURE_BUBBLE);

    lv_obj_t* stripe = lv_obj_create(tile);
    lv_obj_set_pos(stripe, 0, 0);
    lv_obj_set_size(stripe, 4, h - 2);
    lv_obj_set_style_bg_color(stripe, accent, 0);
    lv_obj_set_style_border_width(stripe, 0, 0);
    lv_obj_set_style_radius(stripe, 2, 0);
    lv_obj_clear_flag(stripe, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(stripe, LV_OBJ_FLAG_GESTURE_BUBBLE);

    label(tile, title, 10, 2, &lv_font_montserrat_12, kMuted);
    *value_label = label(tile, "--", 10, 23, &lv_font_montserrat_24);
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
        lv_obj_add_flag(segments[i], LV_OBJ_FLAG_GESTURE_BUBBLE);
    }
}

void Ui::createDash() {
    dash_ = lv_obj_create(nullptr);
    styleScreen(dash_);
    lv_obj_add_event_cb(dash_, gestureEvent, LV_EVENT_GESTURE, nullptr);
    createShiftBar(dash_, dash_shift_);

    label(dash_, "RPM", 332, 42, &lv_font_montserrat_14, kMuted);
    dash_rpm_ = label(dash_, "0000", 300, 58, &lv_font_montserrat_48);
    label(dash_, "GEAR", 610, 42, &lv_font_montserrat_14, kMuted);
    dash_gear_ = label(dash_, "N", 626, 56, &lv_font_montserrat_48, kYellow);

    createValueTile(dash_, "SPEED km/h", 12, 45, 180, 78, &dash_speed_, kBlue);
    createValueTile(dash_, "BOOST / MAP bar", 12, 130, 180, 78, &dash_map_, kBlue);
    createValueTile(dash_, "LAMBDA", 12, 215, 180, 78, &dash_lambda_, kPurple);
    createValueTile(dash_, "CLT C", 12, 300, 180, 78, &dash_clt_, kBlue);
    createValueTile(dash_, "IAT C", 608, 130, 180, 78, &dash_iat_, kBlue);
    createValueTile(dash_, "OIL PRESS bar", 608, 215, 180, 78, &dash_oil_p_, kYellow);
    createValueTile(dash_, "OIL TEMP C", 608, 300, 180, 78, &dash_oil_t_, kYellow);
    createValueTile(dash_, "FUEL PRESS bar", 210, 300, 180, 78, &dash_fuel_p_, kGreen);
    createValueTile(dash_, "BATTERY V", 405, 300, 180, 78, &dash_batt_, kRed);
    createValueTile(dash_, "TPS %", 210, 210, 180, 78, &dash_tps_, kGreen);
    dash_source_ = label(dash_, "DATA SOURCE --", 420, 225, &lv_font_montserrat_14, kMuted);
}

void Ui::createDiag() {
    diag_ = lv_obj_create(nullptr);
    styleScreen(diag_);
    lv_obj_add_event_cb(diag_, gestureEvent, LV_EVENT_GESTURE, nullptr);
    label(diag_, "OPENDASH DIAGNOSTICS", 22, 18, &lv_font_montserrat_28);
    diag_source_ = label(diag_, "v0.2 / DATA SOURCE: --", 24, 56, &lv_font_montserrat_14, kMuted);

    diag_status_ = label(diag_, "Display: --\nTouch: --\nResolution: 800x480", 30, 110, &lv_font_montserrat_20);
    diag_memory_ = label(diag_, "PSRAM: --\nFree heap: --", 315, 110, &lv_font_montserrat_20);
    diag_runtime_ = label(diag_, "Uptime: --\nUI updates: --", 570, 110, &lv_font_montserrat_20);

    diag_can_box_ = lv_obj_create(diag_);
    lv_obj_set_pos(diag_can_box_, 30, 265);
    lv_obj_set_size(diag_can_box_, 740, 105);
    lv_obj_set_style_bg_color(diag_can_box_, lv_color_hex(0x25120F), 0);
    lv_obj_set_style_border_color(diag_can_box_, kRed, 0);
    lv_obj_set_style_border_width(diag_can_box_, 2, 0);
    lv_obj_set_style_radius(diag_can_box_, 8, 0);
    lv_obj_clear_flag(diag_can_box_, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(diag_can_box_, LV_OBJ_FLAG_GESTURE_BUBBLE);
    label(diag_can_box_, "CAN / ECU DATA", 20, 12, &lv_font_montserrat_16, kMuted);
    diag_can_state_ = label(diag_can_box_, "OFFLINE", 20, 38, &lv_font_montserrat_28, kRed);
    diag_can_stats_ = label(diag_can_box_, "1000 kbit/s  RX: 0  BAD: 0", 300, 45, &lv_font_montserrat_16, kMuted);
}

void Ui::createTrack() {
    track_ = lv_obj_create(nullptr);
    styleScreen(track_);
    lv_obj_add_event_cb(track_, gestureEvent, LV_EVENT_GESTURE, nullptr);
    createShiftBar(track_, track_shift_);

    label(track_, "RPM", 324, 46, &lv_font_montserrat_14, kMuted);
    track_rpm_ = label(track_, "0000", 285, 62, &lv_font_montserrat_48);
    label(track_, "GEAR", 612, 46, &lv_font_montserrat_14, kMuted);
    track_gear_ = label(track_, "N", 628, 60, &lv_font_montserrat_48, kYellow);

    createValueTile(track_, "SPEED km/h", 18, 70, 180, 80, &track_speed_, kBlue);
    createValueTile(track_, "BOOST bar", 18, 158, 180, 80, &track_map_, kBlue);
    createValueTile(track_, "LAMBDA", 18, 246, 180, 80, &track_lambda_, kPurple);
    createValueTile(track_, "OIL PRESS bar", 602, 158, 180, 80, &track_oil_p_, kYellow);
    createValueTile(track_, "CLT C", 602, 246, 180, 80, &track_clt_, kBlue);

    label(track_, "LAP TIME (MOCK)", 290, 185, &lv_font_montserrat_14, kMuted);
    label(track_, "00:48.73", 270, 208, &lv_font_montserrat_40);
    label(track_, "BEST (MOCK)  01:35.42", 270, 270, &lv_font_montserrat_16, kPurple);
    label(track_, "DELTA (MOCK)  -0.38", 270, 300, &lv_font_montserrat_16, kGreen);
}

void Ui::createSettings() {
    settings_ = lv_obj_create(nullptr);
    styleScreen(settings_);
    lv_obj_add_event_cb(settings_, gestureEvent, LV_EVENT_GESTURE, nullptr);

    label(settings_, "OPENDASH SETTINGS", 24, 18, &lv_font_montserrat_28);
    label(settings_, "Touch a section to configure", 26, 56, &lv_font_montserrat_14, kMuted);

    for (uint8_t i = 0; i < SettingsScreenModel::sectionCount(); ++i) {
        const int col = i % 2;
        const int row = i / 2;
        lv_obj_t* card = lv_obj_create(settings_);
        lv_obj_set_pos(card, 24 + col * 382, 92 + row * 86);
        lv_obj_set_size(card, 360, 70);
        lv_obj_set_style_bg_color(card, kPanel, 0);
        lv_obj_set_style_border_color(card, lv_color_hex(0x27313C), 0);
        lv_obj_set_style_border_width(card, 1, 0);
        lv_obj_set_style_radius(card, 8, 0);
        lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_add_flag(card, LV_OBJ_FLAG_GESTURE_BUBBLE);
        label(card, SettingsScreenModel::sectionLabel(i), 16, 18, &lv_font_montserrat_20,
              i == 0 ? kBlue : kText);
    }
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

void Ui::update(const VehicleState& s, const RuntimeDiagnostics& d,
                const TelemetryRuntimeStatus& telemetry) {
    char buf[128];
    lv_label_set_text_fmt(dash_rpm_, "%u", s.rpm);
    lv_label_set_text_fmt(dash_gear_, "%d", static_cast<int>(s.gear));
    lv_label_set_text_fmt(dash_speed_, "%.0f", s.speed_kph);
    lv_label_set_text_fmt(dash_map_, "%.2f", s.map_bar);
    lv_label_set_text_fmt(dash_lambda_, "%.2f", s.lambda);
    lv_label_set_text_fmt(dash_clt_, "%.0f", s.clt_c);
    lv_label_set_text_fmt(dash_iat_, "%.0f", s.iat_c);
    lv_label_set_text_fmt(dash_oil_p_, "%.1f", s.oil_pressure_bar);
    lv_label_set_text_fmt(dash_oil_t_, "%.0f", s.oil_temp_c);
    lv_label_set_text_fmt(dash_fuel_p_, "%.1f", s.fuel_pressure_bar);
    lv_label_set_text_fmt(dash_batt_, "%.1f", s.battery_v);
    lv_label_set_text_fmt(dash_tps_, "%.0f", s.tps_percent);
    updateShiftBar(dash_shift_, s.rpm);

    if (telemetry.source == DataSource::Mock) {
        lv_label_set_text(dash_source_, "MOCK TELEMETRY");
        lv_obj_set_style_text_color(dash_source_, kMuted, 0);
    } else {
        std::snprintf(buf, sizeof(buf), "%s CAN / %s", dataSourceName(telemetry.source), telemetry.can_online ? "ONLINE" : "OFFLINE");
        lv_label_set_text(dash_source_, buf);
        lv_obj_set_style_text_color(dash_source_, telemetry.can_online ? kGreen : kRed, 0);
    }

    std::snprintf(buf, sizeof(buf), "Display: %s\nTouch: %s\nResolution: 800x480", d.display_ok ? "OK" : "FAIL", d.touch_ok ? "OK" : "NO");
    lv_label_set_text(diag_status_, buf);
    std::snprintf(buf, sizeof(buf), "PSRAM: %.1f MB\nFree heap: %u KB", static_cast<double>(d.psram_total) / (1024.0 * 1024.0), static_cast<unsigned>(d.free_heap / 1024U));
    lv_label_set_text(diag_memory_, buf);
    std::snprintf(buf, sizeof(buf), "Uptime: %u s\nUI updates: %u", static_cast<unsigned>(d.uptime_ms / 1000U), static_cast<unsigned>(d.ui_updates));
    lv_label_set_text(diag_runtime_, buf);
    std::snprintf(buf, sizeof(buf), "v0.2 / DATA SOURCE: %s", dataSourceName(telemetry.source));
    lv_label_set_text(diag_source_, buf);

    const bool online = telemetry.can_driver_running && telemetry.can_online;
    const bool driver_ready = telemetry.can_driver_running;
    lv_label_set_text(diag_can_state_, online ? "ONLINE" : (driver_ready ? "WAITING FOR ECU" : "DRIVER ERROR"));
    const lv_color_t can_color = online ? kGreen : (driver_ready ? kYellow : kRed);
    lv_obj_set_style_text_color(diag_can_state_, can_color, 0);
    lv_obj_set_style_border_color(diag_can_box_, can_color, 0);
    lv_obj_set_style_bg_color(diag_can_box_, online ? lv_color_hex(0x0D2416) : lv_color_hex(0x25120F), 0);
    std::snprintf(buf, sizeof(buf), "%u kbit/s  RX: %u  BAD: %u", static_cast<unsigned>(telemetry.can_bitrate / 1000U), static_cast<unsigned>(telemetry.received_frames), static_cast<unsigned>(telemetry.invalid_frames));
    lv_label_set_text(diag_can_stats_, buf);

    lv_label_set_text_fmt(track_rpm_, "%u", s.rpm);
    lv_label_set_text_fmt(track_gear_, "%d", static_cast<int>(s.gear));
    lv_label_set_text_fmt(track_speed_, "%.0f", s.speed_kph);
    lv_label_set_text_fmt(track_map_, "%.2f", s.map_bar);
    lv_label_set_text_fmt(track_lambda_, "%.2f", s.lambda);
    lv_label_set_text_fmt(track_oil_p_, "%.1f", s.oil_pressure_bar);
    lv_label_set_text_fmt(track_clt_, "%.0f", s.clt_c);
    updateShiftBar(track_shift_, s.rpm);
}

void Ui::gestureEvent(lv_event_t* event) {
    if (instance_ == nullptr || lv_event_get_code(event) != LV_EVENT_GESTURE) return;
    lv_indev_t* indev = lv_indev_get_act();
    if (indev == nullptr) return;
    const GestureDirection direction = gestureFromLvgl(lv_indev_get_gesture_dir(indev));
    const UiPage next = GestureNavigation::navigate(instance_->current_page_, direction);
    if (next != instance_->current_page_) instance_->load(next);
}

void Ui::load(UiPage page) {
    current_page_ = page;
    switch (page) {
        case UiPage::Dash: lv_scr_load(dash_); break;
        case UiPage::Track: lv_scr_load(track_); break;
        case UiPage::Diag: lv_scr_load(diag_); break;
        case UiPage::Settings: lv_scr_load(settings_); break;
    }
}
