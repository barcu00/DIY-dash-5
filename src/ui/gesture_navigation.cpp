#include "gesture_navigation.h"

#include <cstdlib>

GestureDirection GestureNavigation::classify(int16_t delta_x, int16_t delta_y, uint32_t duration_ms) {
    constexpr int16_t kMinHorizontalTravel = 70;
    constexpr uint32_t kMaxSwipeDurationMs = 700U;

    if (duration_ms > kMaxSwipeDurationMs) {
        return GestureDirection::None;
    }

    const int32_t abs_x = std::abs(static_cast<int>(delta_x));
    const int32_t abs_y = std::abs(static_cast<int>(delta_y));

    // Horizontal travel must be at least 1.25x vertical travel. Integer
    // arithmetic keeps the recognizer deterministic on native and ESP32 builds.
    if (abs_x < kMinHorizontalTravel || abs_x * 4 < abs_y * 5) {
        return GestureDirection::None;
    }

    return delta_x < 0 ? GestureDirection::Left : GestureDirection::Right;
}

UiPage GestureNavigation::navigate(UiPage current, GestureDirection direction) {
    switch (direction) {
        case GestureDirection::Left:
            switch (current) {
                case UiPage::Dash1: return UiPage::Dash2;
                case UiPage::Dash2: return UiPage::Dash3;
                case UiPage::Dash3: return UiPage::Track;
                case UiPage::Track: return UiPage::Diag;
                case UiPage::Diag: return UiPage::Settings;
                case UiPage::Settings: return UiPage::Dash1;
            }
            break;
        case GestureDirection::Right:
            switch (current) {
                case UiPage::Dash1: return UiPage::Settings;
                case UiPage::Settings: return UiPage::Diag;
                case UiPage::Diag: return UiPage::Track;
                case UiPage::Track: return UiPage::Dash3;
                case UiPage::Dash3: return UiPage::Dash2;
                case UiPage::Dash2: return UiPage::Dash1;
            }
            break;
        case GestureDirection::None:
        case GestureDirection::Up:
        case GestureDirection::Down:
            break;
    }
    return current;
}
