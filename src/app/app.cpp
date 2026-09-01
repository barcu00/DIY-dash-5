#include "app.h"

#include <Arduino.h>

namespace {
float registryValueOr(const ParameterRegistry& registry, ParameterId id, float fallback = 0.0f) {
    const ParameterValue& value = registry.value(id);
    return value.valid ? value.value : fallback;
}

const char* sourceName(DataSource source) {
    switch (source) {
        case DataSource::Mock: return "MOCK";
        case DataSource::Ecumaster: return "ECUMASTER";
        case DataSource::Rusefi: return "RUSEFI";
    }
    return "UNKNOWN";
}

lv_obj_t* createBootScreen() {
    lv_obj_t* screen = lv_obj_create(nullptr);
    lv_obj_set_style_bg_color(screen, lv_color_hex(0x07090D), 0);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, 0);
    lv_obj_clear_flag(screen, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* title = lv_label_create(screen);
    lv_label_set_text(title, "OPENDASH");
    lv_obj_set_style_text_color(title, lv_color_hex(0xF2F5F7), 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_28, 0);
    lv_obj_align(title, LV_ALIGN_CENTER, 0, -22);

    lv_obj_t* state = lv_label_create(screen);
    lv_label_set_text(state, "BOOT / DISPLAY OK");
    lv_obj_set_style_text_color(state, lv_color_hex(0x39D353), 0);
    lv_obj_set_style_text_font(state, &lv_font_montserrat_14, 0);
    lv_obj_align(state, LV_ALIGN_CENTER, 0, 24);

    return screen;
}
}

bool App::begin() {
    Serial.println("[OpenDash] Stage 1/5: board/display init");
    if (!board_.begin()) {
        Serial.println("[OpenDash] FATAL: display subsystem unavailable");
        return false;
    }

    // Render a minimal frame before touching NVS, data-source setup or CAN.
    // If a later subsystem fails on real hardware the display remains visibly alive
    // instead of presenting a completely black screen.
    Serial.println("[OpenDash] Stage 2/5: rendering boot frame");
    if (!board_.lock()) {
        Serial.println("[OpenDash] FATAL: cannot lock LVGL for boot frame");
        return false;
    }
    lv_obj_t* boot_screen = createBootScreen();
    lv_scr_load(boot_screen);
    board_.unlock();
    for (uint8_t i = 0; i < 3U; ++i) {
        board_.service();
        delay(10);
    }
    Serial.println("[OpenDash] Boot frame flushed");

    Serial.println("[OpenDash] Stage 3/5: loading persistent configuration");
    if (config_store_.load(config_)) {
        Serial.println("[OpenDash] Configuration loaded from NVS");
    } else {
        config_ = AppConfig::defaults();
        Serial.println("[OpenDash] Using factory configuration");
    }
    config_.validate();
    applyConfig();

    Serial.printf("[OpenDash] Data source: %s, EMU base ID 0x%03X, timeout %u ms\n",
                  sourceName(config_.data_source),
                  static_cast<unsigned>(config_.ecumaster_base_id),
                  static_cast<unsigned>(config_.can_timeout_ms));

    Serial.println("[OpenDash] Stage 4/5: creating full UI");
    if (!board_.lock()) {
        Serial.println("[OpenDash] FATAL: cannot lock LVGL for full UI");
        return false;
    }

    ui_.begin(config_, registry_, config_store_);
    ui_.update(legacyVehicleState(), board_.diagnostics(), telemetryRuntimeStatus());
    board_.unlock();

    // Force the first complete dashboard frame to the LCD before CAN starts.
    for (uint8_t i = 0; i < 3U; ++i) {
        board_.service();
        delay(10);
    }
    Serial.println("[OpenDash] Full UI first frame flushed");

    Serial.println("[OpenDash] Stage 5/5: starting CAN");
    if (config_.data_source != DataSource::Mock) {
        if (can_.begin(config_.can_bitrate)) {
            Serial.printf("[OpenDash] CAN receiver ready: %u kbit/s, TX GPIO15, RX GPIO16\n",
                          static_cast<unsigned>(config_.can_bitrate / 1000U));
        } else {
            Serial.println("[OpenDash] CAN receiver unavailable; ECU data remains offline");
        }
    } else {
        Serial.println("[OpenDash] CAN receiver not started in MOCK mode");
    }

    ready_ = true;
    last_ui_update_ms_ = millis();
    Serial.println("[OpenDash] UI ready - persistent configuration active");
    return true;
}

void App::applyConfig() {
    data_source_.setSource(config_.data_source);
    data_source_.setEcumasterBaseId(config_.ecumaster_base_id);
    data_source_.setTimeoutMs(config_.can_timeout_ms);
    data_source_.update(0U);
    alarm_manager_.resetAll();

    for (uint16_t i = 0; i < AppConfig::kParameterCount; ++i) {
        registry_.setPickerVisible(static_cast<ParameterId>(i), config_.parameter_visible[i]);
    }
}

void App::loop() {
    if (!ready_) {
        // Keep servicing LVGL even after a partial startup failure so a diagnostic
        // boot frame remains visible instead of going black.
        board_.service();
        delay(50);
        return;
    }

    const uint32_t now = millis();

    CanFrame frame{};
    for (uint8_t processed = 0; processed < 32U && can_.poll(frame); ++processed) {
        data_source_.handleCanFrame(frame, now);
    }
    data_source_.update(now);
    alarm_runtime_.update(registry_, config_, alarm_manager_, now);

    if (now - last_ui_update_ms_ >= 50U) {
        if (board_.lock()) {
            board_.incrementUiUpdates();
            ui_.update(legacyVehicleState(), board_.diagnostics(), telemetryRuntimeStatus());
            board_.unlock();
        }
        last_ui_update_ms_ = now;
    }

    board_.service();
    delay(2);
}

VehicleState App::legacyVehicleState() const {
    VehicleState state{};
    state.rpm = static_cast<uint16_t>(registryValueOr(registry_, ParameterId::Rpm, 0.0f));
    state.gear = static_cast<int8_t>(registryValueOr(registry_, ParameterId::Gear, 0.0f));
    state.speed_kph = registryValueOr(registry_, ParameterId::VehicleSpeed);
    state.map_bar = registryValueOr(registry_, ParameterId::Map);
    state.lambda = registryValueOr(registry_, ParameterId::Lambda, 1.0f);
    state.clt_c = registryValueOr(registry_, ParameterId::Clt);
    state.iat_c = registryValueOr(registry_, ParameterId::Iat);
    state.oil_pressure_bar = registryValueOr(registry_, ParameterId::OilPressure);
    state.oil_temp_c = registryValueOr(registry_, ParameterId::OilTemperature);
    state.fuel_pressure_bar = registryValueOr(registry_, ParameterId::FuelPressure);
    state.battery_v = registryValueOr(registry_, ParameterId::BatteryVoltage);
    state.tps_percent = registryValueOr(registry_, ParameterId::Tps);
    return state;
}

TelemetryRuntimeStatus App::telemetryRuntimeStatus() const {
    TelemetryRuntimeStatus status{};
    status.source = data_source_.source();
    status.can_driver_running = can_.running();
    status.can_online = data_source_.canOnline();
    status.can_bitrate = config_.can_bitrate;
    status.received_frames = data_source_.receivedFrameCount();
    status.invalid_frames = data_source_.invalidFrameCount();
    return status;
}
