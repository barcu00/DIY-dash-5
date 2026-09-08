#include "frame_scheduler.h"

void PeriodicDeadline::reset(uint32_t now_ms, uint32_t interval_ms) {
    interval_ms_ = interval_ms == 0U ? 1U : interval_ms;
    next_ms_ = now_ms + interval_ms_;
}

bool PeriodicDeadline::take(uint32_t now_ms) {
    if (static_cast<int32_t>(now_ms - next_ms_) < 0) {
        return false;
    }

    const uint32_t elapsed = now_ms - next_ms_;
    const uint32_t periods = elapsed / interval_ms_ + 1U;
    next_ms_ += periods * interval_ms_;
    return true;
}

void FrameScheduler::reset(uint32_t now_ms) {
    render_.reset(now_ms, kRenderIntervalMs);
    shift_.reset(now_ms, kShiftIntervalMs);
}

bool FrameScheduler::takeRender(uint32_t now_ms) {
    return render_.take(now_ms);
}

bool FrameScheduler::takeShift(uint32_t now_ms) {
    return shift_.take(now_ms);
}
