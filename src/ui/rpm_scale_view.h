#pragma once
#include <array>
#include <lvgl.h>
#include "settings/app_config.h"
#include "telemetry/vehicle_state.h"
#include "ui/display_signal_filter.h"

// Static scale and dynamic needle/blocks are separate drawing layers. No LVGL
// object per tick or LED, and no rebuilding widgets during live telemetry.
class RpmScaleView {
public:
    void create(lv_obj_t* parent);
    void apply(DashboardLayout layout, uint16_t maximum);
    void update(const SignalValue& rpm, uint32_t now_ms,
                const ShiftLightConfig& shift);
private:
    static void drawScale(lv_event_t* event);
    static void drawIndicator(lv_event_t* event);
    lv_point_t needleEnd(uint16_t fill) const;
    void invalidateNeedle(uint16_t fill);
    void invalidateArc(uint16_t previous, uint16_t current);
    void invalidateModernBand();
    void invalidateBlocks(uint16_t previous, uint16_t current);
    std::array<std::array<lv_point_t, 4>, 36> strip_blocks_{};
    lv_obj_t* root_ = nullptr;
    lv_obj_t* indicator_ = nullptr;
    lv_obj_t* value_ = nullptr;
    lv_obj_t* caption_ = nullptr;
    DashboardLayout layout_ = DashboardLayout::ClassicDash;
    uint16_t maximum_ = 10000U;
    uint16_t yellow_from_ = 5500U;
    uint16_t red_from_ = 7000U;
    uint16_t fill_ = 0U;
    bool valid_ = false;
    bool flashing_ = false;
    bool red_phase_ = false;
    bool initialized_ = false;
    uint32_t last_update_ms_ = 0U;
    DisplaySignalFilter filter_{};
    std::array<char, 16> last_text_{};
};
