#include "app.h"

#include <Arduino.h>

#include "ecu/can_profile_registry.h"
#include "odometer/distance_source.h"

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

    beginOdometer(now);
    ui_.setOdometer(&odometer_);
    ui_.begin(config_, board_);
    telemetry_.update(now);
    const UiRuntimeStatus status = runtimeStatus(now);
    const CompositeTelemetryView combined = telemetryView();
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
    serviceConfigCommit(now);
    if (ui_.takeRaceChronoRestart()) racechrono_runtime_.restart(now);
    CanFrame frame;
    // Drain enough per pass to keep up with ~2 kfps buses while a render
    // pass (VSYNC wait) holds the loop for tens of milliseconds.
    for (uint16_t drained = 0U;
         drained < DashboardConfig::kCanDrainPerLoop && can_.poll(frame);
         ++drained) {
        telemetry_.accept(frame, now);
    }
    racechrono_runtime_.service(now);
    telemetry_.update(now);
    serviceOdometer(now);

    if (frame_scheduler_.takeShift(now)) {
        const CompositeTelemetryView combined = telemetryView();
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
            const CompositeTelemetryView combined = telemetryView();
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

// Kept out of loop() so its locals do not sit on the stack underneath the
// LVGL call chain. The request is read in place from the UI commit model
// (no 7 KB copy) and only the engine-relevant fields of the previous
// configuration are remembered.
void App::serviceConfigCommit(uint32_t now_ms) {
    const ConfigCommitRequest* commit = ui_.takeConfigCommit();
    if (commit == nullptr) return;
    const DataSource previous_source = config_.data_source;
    const auto previous_profile = config_.can.profile_id;
    const uint32_t previous_bitrate = config_.can.bitrate;
    const uint32_t previous_timeout = config_.can.timeout_ms;
    const bool reconfigure = commit->reconfigure_runtime;
    const uint32_t revision = commit->revision;
    const bool saved = commit->kind == ConfigCommitKind::FactoryReset
                           ? config_repository_.reset(config_)
                           : config_repository_.saveCandidate(
                                 commit->candidate, config_);
    if (saved && reconfigure) {
        const bool configure_engine =
            previous_source != config_.data_source ||
            previous_profile != config_.can.profile_id ||
            previous_bitrate != config_.can.bitrate ||
            previous_timeout != config_.can.timeout_ms;
        applyRuntimeConfig(now_ms, configure_engine);
    }
    if (board_.lock()) {
        ui_.completeConfigCommit(revision, saved);
        board_.unlock();
    }
}

CompositeTelemetryView App::telemetryView() const {
    return CompositeTelemetryView(telemetry_.state(),
                                  racechrono_runtime_.telemetry(), &odometer_);
}

void App::beginOdometer(uint32_t now_ms) {
    OdometerRecord record{};
    const OdometerLoadResult result = odometer_store_.load(record);
    if (result == OdometerLoadResult::Loaded) odometer_.restore(record, now_ms);
    odometer_storage_ready_ = result != OdometerLoadResult::Unavailable;
    odometer_load_retry_ms_ = now_ms + OdometerModel::kRetryAfterFailureMs;
    Serial.printf("[ODO] %s, %.1f km\n",
                  result == OdometerLoadResult::Loaded ? "loaded"
                  : result == OdometerLoadResult::Empty ? "new"
                  : result == OdometerLoadResult::Corrupt ? "corrupt record, reset"
                                                          : "storage unavailable",
                  odometer_.odometerKm());
}

void App::serviceOdometer(uint32_t now_ms) {
    const CompositeTelemetryView view = telemetryView();
    odometer_.integrate(selectDistanceSpeed(odometer_.settings(), view,
                                            telemetry_.demoActive()),
                        now_ms);
    if (!odometer_storage_ready_) {
        // Never overwrite a record we could not read; retry the load and
        // merge the distance driven since boot once storage is back.
        if (static_cast<int32_t>(now_ms - odometer_load_retry_ms_) < 0) return;
        odometer_load_retry_ms_ = now_ms + OdometerModel::kRetryAfterFailureMs;
        OdometerRecord record{};
        const OdometerLoadResult result = odometer_store_.load(record);
        if (result == OdometerLoadResult::Unavailable) return;
        if (result == OdometerLoadResult::Loaded) odometer_.mergeStored(record, now_ms);
        odometer_storage_ready_ = true;
    }
    if (!odometer_.persistDue(now_ms)) return;
    if (odometer_store_.save(odometer_.record())) odometer_.markPersisted(now_ms);
    else odometer_.persistFailed(now_ms);
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
    const CanHealth health = can_.health();
    status.can_dropped_frames = health.dropped_frames;
    status.can_bus_errors = health.bus_errors;
    status.can_error_passive = health.error_passive;
    status.racechrono_connection = racechrono_runtime_.state();
    status.racechrono = racechrono_runtime_.status(now_ms);
    return status;
}
