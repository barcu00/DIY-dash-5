#pragma once

#include <cstdint>

class PeriodicDeadline {
public:
    void reset(uint32_t now_ms, uint32_t interval_ms);
    bool take(uint32_t now_ms);

private:
    uint32_t next_ms_ = 0U;
    uint32_t interval_ms_ = 1U;
};

class FrameScheduler {
public:
    static constexpr uint32_t kRenderIntervalMs = 25U;
    static constexpr uint32_t kShiftIntervalMs = 5U;

    void reset(uint32_t now_ms);
    bool takeRender(uint32_t now_ms);
    bool takeShift(uint32_t now_ms);

private:
    PeriodicDeadline render_{};
    PeriodicDeadline shift_{};
};
