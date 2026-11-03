#pragma once

#include <lvgl.h>

#include "settings/app_config.h"
#include "telemetry/vehicle_state.h"
#include "ui/display_signal_filter.h"
#include "ui/tach_bar_model.h"

// Full-width tachometer bar with scale, used as the shift light on the
// tile layouts. One LVGL object, custom drawn; only the changed part of the
// bar is invalidated.
class TachBarView {
public:
    void create(lv_obj_t* parent);
    void apply(DashboardLayout layout, uint16_t scale_max);
    void update(const SignalValue& rpm, uint32_t now_ms,
                const ShiftLightConfig& shift);

private:
    static void drawEvent(lv_event_t* event);
    void invalidateFill(uint16_t previous, uint16_t current);

    lv_obj_t* root_ = nullptr;
    uint16_t scale_max_ = 8000U;
    ShiftLightConfig shift_{};
    TachBarState state_{};
    bool initialized_ = false;
    DisplaySignalFilter filter_{};
};
