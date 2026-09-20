#pragma once

#include <cstddef>
#include <cstdint>

#include "racechrono_event_queue.h"
#include "racechrono_session.h"

class RaceChronoTransport {
public:
    virtual ~RaceChronoTransport() = default;
    virtual bool startAdvertising() = 0;
    virtual void stopAdvertising() = 0;
    virtual void disconnect() = 0;
    virtual bool indicate(const uint8_t* data, std::size_t size) = 0;
};

class RaceChronoRuntime {
public:
    explicit RaceChronoRuntime(RaceChronoTransport& transport);

    void setEnabled(bool enabled, uint32_t now_ms);
    bool enqueueEvent(const RaceChronoEvent& event);
    void service(uint32_t now_ms);
    void restart(uint32_t now_ms);

    RaceChronoTelemetry& telemetry();
    const RaceChronoTelemetry& telemetry() const;
    RaceChronoConnectionState state() const;
    RaceChronoRuntimeStatus status(uint32_t now_ms) const;
    uint32_t droppedEvents() const;

private:
    void executeActions();

    RaceChronoTransport& transport_;
    RaceChronoTelemetry telemetry_{};
    RaceChronoSession session_{telemetry_};
    RaceChronoEventQueue<32U> events_{};
    bool enabled_ = false;
};
