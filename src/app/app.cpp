#include "app.h"

#include <Arduino.h>

bool App::begin() {
    if (!board_.begin()) {
        Serial.println("[DIY Dash] FATAL: display subsystem unavailable");
        return false;
    }

    const uint32_t now = millis();
    const bool can_ready = can_.begin(DashboardConfig::kCanBitrate);
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

    ui_.begin();
    telemetry_.update(now);
    alarm_summary_ = alarms_.evaluate(telemetry_.state());
    const UiRuntimeStatus status{
        telemetry_.canStatus(), alarm_summary_, telemetry_.demoActive(),
        DashboardConfig::kCanBitrate, DashboardConfig::kCanTimeoutMs,
        telemetry_.mappingCount(),
        can_.receivedFrames(), can_.rejectedFrames()};
    ui_.update(telemetry_.state(), board_.diagnostics(), status);
    board_.unlock();

    ready_ = true;
    last_ui_update_ms_ = now;
    Serial.println("[DIY Dash] UI ready - DASH / TRACK / DIAG / SETTINGS");
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
        alarm_summary_ = alarms_.evaluate(telemetry_.state());
        if (board_.lock()) {
            board_.incrementUiUpdates();
            const UiRuntimeStatus status{
                telemetry_.canStatus(), alarm_summary_, telemetry_.demoActive(),
                DashboardConfig::kCanBitrate, DashboardConfig::kCanTimeoutMs,
                telemetry_.mappingCount(),
                can_.receivedFrames(), can_.rejectedFrames()};
            ui_.update(telemetry_.state(), board_.diagnostics(), status);
            board_.unlock();
        }
        last_ui_update_ms_ = now;
    }

    board_.service();
    delay(2);
}
