#pragma once

#include <cstddef>
#include <cstdint>

#include "racechrono_event_queue.h"
#include "racechrono_runtime.h"

class RaceChronoBleTransport final : public RaceChronoTransport {
public:
    RaceChronoBleTransport();
    ~RaceChronoBleTransport() override;

    bool begin();
    bool startAdvertising() override;
    void stopAdvertising() override;
    void disconnect() override;
    bool indicate(const uint8_t* data, std::size_t size) override;
    bool pollEvent(RaceChronoEvent& event) override;
    bool restart();

private:
    struct Impl;
    Impl* impl_ = nullptr;
    RaceChronoEventQueue<32U> events_{};
};
