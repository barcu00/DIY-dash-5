#include "app.h"

#include <Arduino.h>

bool App::begin() {
    if (!board_.begin()) {
        Serial.println("[OpenDash] FATAL: display subsystem unavailable");
        return false;
    }

    if (!board_.lock()) {
        Serial.println("[OpenDash] FATAL: cannot lock LVGL");
        return false;
    }

    ui_.begin();
    telemetry_.reset();
    ui_.update(telemetry_.state(), board_.diagnostics());
    board_.unlock();

    ready_ = true;
    last_ui_update_ms_ = millis();
    Serial.println("[OpenDash] UI ready - DASH / DIAG / TRACK");
    return true;
}

void App::loop() {
    if (!ready_) {
        delay(250);
        return;
    }

    const uint32_t now = millis();
    if (now - last_ui_update_ms_ >= 50U) {
        telemetry_.update(now);
        if (board_.lock()) {
            board_.incrementUiUpdates();
            ui_.update(telemetry_.state(), board_.diagnostics());
            board_.unlock();
        }
        last_ui_update_ms_ = now;
    }

    board_.service();
    delay(2);
}
