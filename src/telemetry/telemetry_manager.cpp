#include "telemetry_manager.h"

TelemetryManager::TelemetryManager(const EcuCanDecoder& decoder,
                                   uint32_t can_timeout_ms)
    : decoder_(decoder), can_timeout_ms_(can_timeout_ms) {
    can_state_.reset(DataSource::Can);
    empty_state_.reset(DataSource::None);
    for (std::size_t i = 0; i < timeouts_.size(); ++i) {
        timeouts_[i] = decoder_.timeoutFor(static_cast<VehicleSignal>(i));
    }
}

void TelemetryManager::selectSource(DataSource source, uint32_t now_ms) {
    selected_source_ = source == DataSource::Can ? DataSource::Can
                                                 : DataSource::Demo;
    demo_active_ = selected_source_ == DataSource::Demo;
    has_valid_frame_ = false;
    started_ms_ = now_ms;
    last_valid_frame_ms_ = now_ms;
    can_state_.reset(DataSource::Can);
    empty_state_.reset(DataSource::None);
    can_status_ = selected_source_ == DataSource::Can
                      ? (can_initialized_ ? CanStatus::Waiting
                                          : CanStatus::InitFailed)
                      : CanStatus::Disabled;
}

void TelemetryManager::setCanTimeout(uint32_t timeout_ms) {
    can_timeout_ms_ = timeout_ms;
}

void TelemetryManager::setCanInitialized(bool initialized, uint32_t now_ms) {
    can_initialized_ = initialized;
    started_ms_ = now_ms;
    if (selected_source_ == DataSource::Can) {
        can_status_ = initialized ? CanStatus::Waiting : CanStatus::InitFailed;
    } else {
        can_status_ = CanStatus::Disabled;
    }
}

bool TelemetryManager::accept(const CanFrame& frame, uint32_t now_ms) {
    if (selected_source_ != DataSource::Can || !can_initialized_ ||
        !decoder_.decode(frame, can_state_, now_ms)) {
        return false;
    }

    has_valid_frame_ = true;
    last_valid_frame_ms_ = now_ms;
    can_status_ = CanStatus::Online;
    demo_active_ = false;
    return true;
}

void TelemetryManager::update(uint32_t now_ms) {
    if (selected_source_ == DataSource::Demo) {
        demo_active_ = true;
        can_status_ = CanStatus::Disabled;
        demo_.update(now_ms);
        return;
    }

    can_state_.invalidateStale(now_ms, timeouts_.data());

    const uint32_t reference_ms = has_valid_frame_ ? last_valid_frame_ms_ : started_ms_;
    const bool timed_out = now_ms - reference_ms > can_timeout_ms_;
    if (can_initialized_) {
        can_status_ = has_valid_frame_
                          ? (timed_out ? CanStatus::Offline : CanStatus::Online)
                          : (timed_out ? CanStatus::Offline : CanStatus::Waiting);
    }

    demo_active_ = false;
    if (can_status_ != CanStatus::Online) {
        empty_state_.reset(DataSource::None);
    }
}

const VehicleState& TelemetryManager::state() const {
    if (demo_active_) {
        return demo_.state();
    }
    if (can_status_ == CanStatus::Online) {
        return can_state_;
    }
    return empty_state_;
}

CanStatus TelemetryManager::canStatus() const {
    return can_status_;
}

bool TelemetryManager::demoActive() const {
    return demo_active_;
}

DataSource TelemetryManager::selectedSource() const {
    return selected_source_;
}

std::size_t TelemetryManager::mappingCount() const {
    return decoder_.definitionCount();
}
