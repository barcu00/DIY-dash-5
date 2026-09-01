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
}

bool App::begin() {
    if (!board_.begin()) {
        Serial.println("[OpenDash] FATAL: display subsystem unavailable");
        return false;
    }

    if (config_store_.load(config_)) {
        Serial.println("[OpenDash] Configuration loaded from NVS");
    } else {
        config_ = AppConfig::defaults();
        Serial.println("[OpenDash] Using factory configuration");
    }
    config_.validate();
    applyConfig();

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

    Serial.printf("[OpenDash] Data source: %s, EMU base ID 0x%03X, timeout %u ms\n",
                  sourceName(config_.data_source),
                  static_cast<unsigned>(config_.ecumaster_base_id),
                  static_cast<unsigned>(config_.can_timeout_ms));

    if (!board_.lock()) {
        Serial.println("[OpenDash] FATAL: cannot lock LVGL");
        return false;
    }

    ui_.begin(config_, registry_, config_store_);
    ui_.update(legacyVehicleState(), board_.diagnostics(), telemetryRuntimeStatus());
    board_.unlock();

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

    for (uint16_t i = 0; i < AppConfig::kParameterCount; ++i) {
        registry_.setPickerVisible(static_cast<ParameterId>(i), config_.parameter_visible[i]);
    }
}

void App::loop() {
    if (!ready_) {
        delay(250);
        return;
    }

    const uint32_t now = millis();

    CanFrame frame{};
    for (uint8_t processed = 0; processed < 32U && can_.poll(frame); ++processed) {
        data_source_.handleCanFrame(frame, now);
    }
    data_source_.update(now);

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
