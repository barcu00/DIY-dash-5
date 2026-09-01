#pragma once

#include <cstdint>

#include "can_frame.h"

class CanDriver {
public:
    bool begin(uint32_t bitrate);
    bool poll(CanFrame& frame);
    void stop();

    bool running() const;
    uint32_t bitrate() const;

private:
    bool running_ = false;
    uint32_t bitrate_ = 0;
};
