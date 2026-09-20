#include "app.h"

#include <Arduino.h>

#include "ecu/can_profile_registry.h"

bool App::begin() {
    config_repository_.load(config_);
    if (!board_.begin()) {
        Serial.println("[DIY Dash] FATAL: display subsystem unavailable");
        return false;
    }
    if (!racechrono_transport_.begin())
        Serial.println("[DIY Dash] WARNING: RaceChrono BLE initialization failed");

    const uint32_t now = millis();
    applyRuntimeConfig(now);
    racechrono_runtime_.service(now);

    if (!board_.lock()) {
        Serial.println("[DIY Dash] FATAL: cannot lock LVGL");
        return false;
    }

    ui_.begin(config_, board_);
    telemetry_.update(now);
    const UiRuntimeStatus status = runtimeStatus(now);
    const CompositeTelemetryView combined(
        telemetry_.state(), racechrono_runtime_.telemetry());
    warnings_.evaluate(config_, combined, now);
    ui_.update(combined, board_.diagnostics(), status, config_, warnings_);
    ui_.updateShiftLight(telemetry_.state(), now, config_.shift);
    board_.unlock();

    if (!board_.beginBuzzer())
        Serial.println("[DIY Dash] WARNING: DO0 buzzer initialization failed");
    buzzer_.begin(millis());
    ready_ = true;
    frame_scheduler_.reset(now);
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
        const AppConfig previous = config_;
        const bool saved = commit.kind == ConfigCommitKind::FactoryReset
                               ? config_repository_.reset(config_)
                               : config_repository_.saveCandidate(
                                     commit.candidate, config_);
        if (saved && commit.reconfigure_runtime) {
            const bool configure_engine =
                previous.data_source != config_.data_source ||
                previous.can.profile_id != config_.can.profile_id ||
                previous.can.bitrate != config_.can.bitrate ||
                previous.can.timeout_ms != config_.can.timeout_ms;
            applyRuntimeConfig(now, configure_engine);
        }
        if (board_.lock()) {
            ui_.completeConfigCommit(commit.revision, saved);
            board_.unlock();
        }
    }
    if (ui_.takeRaceChronoRestart()) racechrono_runtime_.restart(now);
    CanFrame frame;
    for (uint8_t drained = 0U; drained < 32U && can_.poll(frame); ++drained) {
        telemetry_.accept(frame, now);
    }
    racechrono_runtime_.service(now);
    telemetry_.update(now);

    if (frame_scheduler_.takeShift(now)) {
        const CompositeTelemetryView combined(
            telemetry_.state(), racechrono_runtime_.telemetry());
        warnings_.evaluate(config_, combined, now);
        if (board_.lock()) {
            ui_.updateShiftLight(telemetry_.state(), now, config_.shift);
            board_.unlock();
        }
    }

    if (frame_scheduler_.takeRender(now)) {
        if (board_.lock()) {
            board_.incrementUiUpdates();
            const UiRuntimeStatus status = runtimeStatus(now);
            const CompositeTelemetryView combined(
                telemetry_.state(), racechrono_runtime_.telemetry());
            ui_.update(combined, board_.diagnostics(), status, config_, warnings_);
            board_.unlock();
        }
    }

    board_.service();
    if (board_.lock(10)) {
        const uint32_t buzzer_now=millis();
        if(ui_.takeWarningTest())buzzer_test_.request(buzzer_now);
        const bool real_alarm=buzzer_.update(buzzer_now,config_.warning_sound_enabled,
                                           warnings_.nextModal().has_value());
        const bool test=buzzer_test_.update(buzzer_now,config_.warning_sound_enabled);
        board_.setBuzzer(real_alarm || test);
        board_.unlock();
    }
    delay(2);
}

void App::applyRuntimeConfig(uint32_t now_ms, bool configure_engine) {
    racechrono_runtime_.setEnabled(config_.racechrono.enabled, now_ms);
    if (!configure_engine) return;
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
    ui_.setDataContext(config_.data_source, profile);
    Serial.printf("[DIY Dash] Source: %s; profile: %s; CAN listen-only: %s; %u bit/s\n",
                  config_.data_source == DataSource::Can ? "CAN" : "DEMO",
                  profile == nullptr ? "none" : profile->id,
                  can_ready ? "READY" : "INACTIVE",
                  static_cast<unsigned>(config_.can.bitrate));
}

UiRuntimeStatus App::runtimeStatus(uint32_t now_ms) const {
    UiRuntimeStatus status;
    status.can_status = telemetry_.canStatus();
    status.demo_active = telemetry_.demoActive();
    status.can_bitrate = config_.can.bitrate;
    status.can_timeout_ms = config_.can.timeout_ms;
    status.decoder_mappings = telemetry_.mappingCount();
    status.received_frames = can_.receivedFrames();
    status.rejected_frames = can_.rejectedFrames();
    status.racechrono_connection = racechrono_runtime_.state();
    status.racechrono = racechrono_runtime_.status(now_ms);
    return status;
}
