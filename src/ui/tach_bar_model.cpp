#include "tach_bar_model.h"

#include <cmath>

#include "ui/dashboard_layout.h"

TachBarState tachBarState(float rpm, bool valid, uint16_t scale_max,
                          const ShiftLightConfig& shift, uint32_t now_ms) {
    TachBarState state;
    if (!valid || !std::isfinite(rpm) || rpm < 0.0f) return state;
    if (shift.flash_enabled && rpm >= shift.flash_rpm) {
        state.flashing = true;
        state.fill_permille = 1000U;
        state.color = (now_ms % 250U) < 125U ? TachColor::Red : TachColor::Off;
        return state;
    }
    state.fill_permille = rpmScaleFill(rpm, true, normalizedRpmScale(scale_max));
    state.color = rpm >= shift.red_rpm ? TachColor::Red
                : rpm >= shift.start_rpm ? TachColor::Yellow
                : TachColor::Normal;
    return state;
}
