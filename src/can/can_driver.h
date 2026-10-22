#pragma once

#include <cstdint>

#include "can/can_frame.h"

struct CanHealth {
    uint32_t dropped_frames = 0U;  // rx_missed + rx_overrun since begin()
    uint32_t bus_errors = 0U;
    bool error_passive = false;
};

class CanDriver {
public:
    ~CanDriver();

    bool begin(uint32_t bitrate);
    bool poll(CanFrame& frame);
    void stop();

    bool running() const;
    uint32_t bitrate() const;
    uint32_t receivedFrames() const;
    uint32_t rejectedFrames() const;
    // Polled from the driver; cached so the UI can read it cheaply.
    CanHealth health() const;

private:
    bool running_ = false;
    uint32_t bitrate_ = 0U;
    uint32_t received_frames_ = 0U;
    uint32_t rejected_frames_ = 0U;
    mutable CanHealth health_{};
    mutable uint32_t health_polled_ms_ = 0U;
};
