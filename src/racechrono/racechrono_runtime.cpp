#include "racechrono_runtime.h"

RaceChronoRuntime::RaceChronoRuntime(RaceChronoTransport& transport)
    : transport_(transport) {}

void RaceChronoRuntime::setEnabled(bool enabled, uint32_t) {
    if (enabled == enabled_) return;
    enabled_ = enabled;
    has_deferred_action_ = false;
    if (!enabled_) events_.clear();
    session_.setEnabled(enabled_);
}

bool RaceChronoRuntime::enqueueEvent(const RaceChronoEvent& event) {
    return events_.pushFromProducer(event);
}

void RaceChronoRuntime::service(uint32_t now_ms) {
    RaceChronoEvent event;
    for (uint8_t drained = 0U;
         drained < 8U && nextEvent(event); ++drained) {
        session_.onEvent(event, now_ms);
    }
    session_.update(now_ms);
    executeActions();
    telemetry_.updateStale(now_ms);
}

bool RaceChronoRuntime::nextEvent(RaceChronoEvent& event) {
    return events_.popFromConsumer(event) || transport_.pollEvent(event);
}

void RaceChronoRuntime::restart(uint32_t now_ms) {
    if (!enabled_) return;
    events_.clear();
    has_deferred_action_ = false;
    transport_.disconnect();
    session_.onEvent(RaceChronoEvent{RaceChronoEventType::Disconnected},
                     now_ms);
    executeActions();
}

RaceChronoTelemetry& RaceChronoRuntime::telemetry() {
    return telemetry_;
}

const RaceChronoTelemetry& RaceChronoRuntime::telemetry() const {
    return telemetry_;
}

RaceChronoConnectionState RaceChronoRuntime::state() const {
    return session_.state();
}

RaceChronoRuntimeStatus RaceChronoRuntime::status(uint32_t now_ms) const {
    return telemetry_.snapshotStatus(now_ms);
}

uint32_t RaceChronoRuntime::droppedEvents() const {
    return events_.droppedCount();
}

void RaceChronoRuntime::executeActions() {
    if (has_deferred_action_) {
        if (!executeAction(deferred_action_)) return;
        has_deferred_action_ = false;
    }
    RaceChronoAction action;
    while (session_.takeAction(action)) {
        if (!executeAction(action)) {
            deferred_action_ = action;
            has_deferred_action_ = true;
            return;
        }
    }
}

bool RaceChronoRuntime::executeAction(const RaceChronoAction& action) {
    switch (action.type) {
        case RaceChronoActionType::StartAdvertising:
            return transport_.startAdvertising();
        case RaceChronoActionType::StopAdvertising:
            transport_.stopAdvertising();
            return true;
        case RaceChronoActionType::Disconnect:
            transport_.disconnect();
            return true;
        case RaceChronoActionType::Indicate:
            return transport_.indicate(action.packet.bytes.data(),
                                       action.packet.size);
        case RaceChronoActionType::None:
            return true;
    }
    return true;
}
