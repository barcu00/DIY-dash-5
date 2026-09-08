#include "app.h"

#include <Arduino.h>

#include "ecu/can_profile_registry.h"

bool App::begin() {
    config_repository_.load(config_);
    if (!board_.begin()) {
        Serial.println("[DIY Dash] FATAL: display subsystem unavailable");
        return false;
    }

    const uint32_t now = millis();
    applyRuntimeConfig(now);

    if (!board_.lock()) {
        Serial.println("[DIY Dash] FATAL: cannot lock LVGL");
        return false;
    }

    ui_.begin(config_, board_);
    telemetry_.update(now);
    const UiRuntimeStatus status{
        telemetry_.canStatus(), telemetry_.demoActive(),
        config_.can.bitrate, config_.can.timeout_ms,
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
    ConfigCommitRequest commit;
    if (ui_.takeConfigCommit(commit)) {
        const bool saved = commit.kind == ConfigCommitKind::FactoryReset
                               ? config_repository_.reset(config_)
                               : config_repository_.saveCandidate(
                                     commit.candidate, config_);
        if (saved && commit.reconfigure_runtime) {
            applyRuntimeConfig(now);
        }
        if (board_.lock()) {
            ui_.completeConfigCommit(commit.revision, saved);
            board_.unlock();
        }
    }
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
                config_.can.bitrate, config_.can.timeout_ms,
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

void App::applyRuntimeConfig(uint32_t now_ms) {
    can_.stop();
    telemetry_.selectProfile(
        CanProfileRegistry::find(config_.can.profile_id.data()), now_ms);
    telemetry_.setCanTimeout(config_.can.timeout_ms);
    telemetry_.selectSource(config_.data_source, now_ms);
    const bool can_ready = config_.data_source == DataSource::Can
                               ? can_.begin(config_.can.bitrate)
                               : false;
    telemetry_.setCanInitialized(can_ready, now_ms);
    const CanProfile* profile = decoder_.profile();
    Serial.printf("[DIY Dash] Source: %s; profile: %s; CAN listen-only: %s; %u bit/s\n",
                  config_.data_source == DataSource::Can ? "CAN" : "DEMO",
                  profile == nullptr ? "none" : profile->id,
                  can_ready ? "READY" : "INACTIVE",
                  static_cast<unsigned>(config_.can.bitrate));
}
