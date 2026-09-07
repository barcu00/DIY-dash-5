#include "app.h"

#include <Arduino.h>

bool App::begin() {
    if (!board_.begin()) {
        Serial.println("[DIY Dash] FATAL: display subsystem unavailable");
        return false;
    }

    const uint32_t now = millis();
    const DataSource selected_source = DashboardConfig::kDemoEnabled
                                           ? DataSource::Demo
                                           : DataSource::Can;
    telemetry_.selectSource(selected_source, now);
    const bool can_ready = selected_source == DataSource::Can
                               ? can_.begin(DashboardConfig::kCanBitrate)
                               : false;
    telemetry_.setCanInitialized(can_ready, now);
    Serial.printf("[DIY Dash] CAN: %s, TX GPIO%u, RX GPIO%u, %u bit/s\n",
                  can_ready ? "READY" : "INIT FAILED",
                  DashboardConfig::kCanTxGpio, DashboardConfig::kCanRxGpio,
                  static_cast<unsigned>(DashboardConfig::kCanBitrate));
    Serial.printf("[DIY Dash] Decoder mappings: %u; DEMO fallback: %s\n",
                  static_cast<unsigned>(decoder_.definitionCount()),
                  DashboardConfig::kDemoEnabled ? "ENABLED" : "DISABLED");

    if (!board_.lock()) {
        Serial.println("[DIY Dash] FATAL: cannot lock LVGL");
        return false;
    }

    ui_.begin(config_);
    telemetry_.update(now);
    const UiRuntimeStatus status{
        telemetry_.canStatus(), telemetry_.demoActive(),
        DashboardConfig::kCanBitrate, DashboardConfig::kCanTimeoutMs,
        telemetry_.mappingCount(),
        can_.receivedFrames(), can_.rejectedFrames()};
    warnings_.evaluate(config_, telemetry_.state(), now);
    ui_.update(telemetry_.state(), board_.diagnostics(), status, config_, warnings_);
    board_.unlock();

    ready_ = true;
    last_ui_update_ms_ = now;
    Serial.println("[DIY Dash] UI ready - DASH / TRACK / SETTINGS");
    return true;
}

void App::loop() {
    if (!ready_) {
        delay(250);
        return;
    }

    const uint32_t now = millis();
    CanFrame frame;
    for (uint8_t drained = 0U; drained < 32U && can_.poll(frame); ++drained) {
        telemetry_.accept(frame, now);
    }
    telemetry_.update(now);

    if (now - last_ui_update_ms_ >= 50U) {
        warnings_.evaluate(config_, telemetry_.state(), now);
        if (board_.lock()) {
            board_.incrementUiUpdates();
            const UiRuntimeStatus status{
                telemetry_.canStatus(), telemetry_.demoActive(),
                DashboardConfig::kCanBitrate, DashboardConfig::kCanTimeoutMs,
                telemetry_.mappingCount(),
                can_.receivedFrames(), can_.rejectedFrames()};
            ui_.update(telemetry_.state(), board_.diagnostics(), status,
                       config_, warnings_);
            board_.unlock();
        }
        last_ui_update_ms_ = now;
    }

    board_.service();
    delay(2);
}
