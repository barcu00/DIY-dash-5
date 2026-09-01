#pragma once

#include <cstdint>

enum class UiPage : uint8_t {
    Dash = 0,
    Track = 1,
    Diag = 2,
    Settings = 3,
};

enum class GestureDirection : uint8_t {
    None = 0,
    Left,
    Right,
    Up,
    Down,
};

class GestureNavigation {
public:
    static GestureDirection classify(int16_t delta_x, int16_t delta_y, uint32_t duration_ms);
    static UiPage navigate(UiPage current, GestureDirection direction);
};
