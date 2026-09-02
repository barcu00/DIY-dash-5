#include "touch_swipe_recognizer.h"

void TouchSwipeRecognizer::reset() {
    tracking_ = false;
    start_x_ = 0;
    start_y_ = 0;
    last_x_ = 0;
    last_y_ = 0;
    start_ms_ = 0U;
}

GestureDirection TouchSwipeRecognizer::update(bool pressed, int16_t x, int16_t y, uint32_t now_ms) {
    if (pressed) {
        if (!tracking_) {
            tracking_ = true;
            start_x_ = x;
            start_y_ = y;
            start_ms_ = now_ms;
        }
        last_x_ = x;
        last_y_ = y;
        return GestureDirection::None;
    }

    if (!tracking_) {
        return GestureDirection::None;
    }

    const int16_t delta_x = static_cast<int16_t>(last_x_ - start_x_);
    const int16_t delta_y = static_cast<int16_t>(last_y_ - start_y_);
    const uint32_t duration_ms = now_ms - start_ms_;
    reset();
    return GestureNavigation::classify(delta_x, delta_y, duration_ms);
}
