#include "gesture_navigation.h"

#include <cstdlib>

GestureDirection GestureNavigation::classify(int16_t delta_x, int16_t delta_y, uint32_t duration_ms) {
    constexpr int16_t kMinHorizontalTravel = 60;
    constexpr uint32_t kMaxSwipeDurationMs = 700U;

    if (duration_ms > kMaxSwipeDurationMs) {
        return GestureDirection::None;
    }

    const int16_t abs_x = static_cast<int16_t>(std::abs(static_cast<int>(delta_x)));
    const int16_t abs_y = static_cast<int16_t>(std::abs(static_cast<int>(delta_y)));
    if (abs_x < kMinHorizontalTravel || abs_x <= abs_y) {
        return GestureDirection::None;
    }

    return delta_x < 0 ? GestureDirection::Left : GestureDirection::Right;
}

UiPage GestureNavigation::navigate(UiPage current, GestureDirection direction) {
    switch (direction) {
        case GestureDirection::Left:
            switch (current) {
                case UiPage::Dash: return UiPage::Track;
                case UiPage::Track: return UiPage::Diag;
                case UiPage::Diag: return UiPage::Settings;
                case UiPage::Settings: return UiPage::Dash;
            }
            break;
        case GestureDirection::Right:
            switch (current) {
                case UiPage::Dash: return UiPage::Settings;
                case UiPage::Settings: return UiPage::Diag;
                case UiPage::Diag: return UiPage::Track;
                case UiPage::Track: return UiPage::Dash;
            }
            break;
        case GestureDirection::None:
        case GestureDirection::Up:
        case GestureDirection::Down:
            break;
    }
    return current;
}
