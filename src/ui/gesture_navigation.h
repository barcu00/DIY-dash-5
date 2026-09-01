#pragma once

#include <cstdint>

enum class UiPage : uint8_t {
    Dash = 0,
    Diag = 1,
    Track = 2,
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
    static UiPage navigate(UiPage current, GestureDirection direction);
};
