#include "gesture_navigation.h"

UiPage GestureNavigation::navigate(UiPage current, GestureDirection direction) {
    switch (direction) {
        case GestureDirection::Left:
            switch (current) {
                case UiPage::Dash: return UiPage::Diag;
                case UiPage::Diag: return UiPage::Track;
                case UiPage::Track: return UiPage::Dash;
            }
            break;
        case GestureDirection::Right:
            switch (current) {
                case UiPage::Dash: return UiPage::Track;
                case UiPage::Diag: return UiPage::Dash;
                case UiPage::Track: return UiPage::Diag;
            }
            break;
        case GestureDirection::None:
        case GestureDirection::Up:
        case GestureDirection::Down:
            break;
    }
    return current;
}
