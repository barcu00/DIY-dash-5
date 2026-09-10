#pragma once

#include <cstdint>

#include "can/can_frame.h"

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

private:
    bool running_ = false;
    uint32_t bitrate_ = 0U;
    uint32_t received_frames_ = 0U;
    uint32_t rejected_frames_ = 0U;
};
