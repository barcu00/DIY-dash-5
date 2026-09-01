#include "app.h"

#include <Arduino.h>

namespace {
float registryValueOr(const ParameterRegistry& registry, ParameterId id, float fallback = 0.0f) {
    const ParameterValue& value = registry.value(id);
    return value.valid ? value.value : fallback;
}
}

bool App::begin() {
    if (!board_.begin()) {
        Serial.println("[OpenDash] FATAL: display subsystem unavailable");
        return false;
    }

    data_source_.setSource(DataSource::Ecumaster);
    data_source_.setEcumasterBaseId(0x600U);
    data_source_.setTimeoutMs(500U);
    data_source_.update(0U);

    if (can_.begin(1000000U)) {
        Serial.println("[OpenDash] CAN receiver ready: 1000 kbit/s, TX GPIO15, RX GPIO16");
        Serial.println("[OpenDash] Data source: ECUMaster, base ID 0x600, timeout 500 ms");
    } else {
        Serial.println("[OpenDash] CAN receiver unavailable; ECUMaster data remains offline");
    }

    if (!board_.lock()) {
        Serial.println("[OpenDash] FATAL: cannot lock LVGL");
        return false;
    }

    ui_.begin();
    ui_.update(legacyVehicleState(), board_.diagnostics(), telemetryRuntimeStatus());
    board_.unlock();

    ready_ = true;
    last_ui_update_ms_ = millis();
    Serial.println("[OpenDash] UI ready - parameter registry active");
    return true;
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
    status.can_bitrate = can_.bitrate();
    status.received_frames = data_source_.receivedFrameCount();
    status.invalid_frames = data_source_.invalidFrameCount();
    return status;
}
