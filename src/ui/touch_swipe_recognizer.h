#pragma once

#include <cstdint>

#include "ui/gesture_navigation.h"

class TouchSwipeRecognizer {
public:
    void reset();
    GestureDirection update(bool pressed, int16_t x, int16_t y, uint32_t now_ms);

private:
    bool tracking_ = false;
    int16_t start_x_ = 0;
    int16_t start_y_ = 0;
    int16_t last_x_ = 0;
    int16_t last_y_ = 0;
    uint32_t start_ms_ = 0U;
};
