#pragma once
#include <array>
#include <lvgl.h>
#include "settings/app_config.h"
class ShiftLightView {
public:
    void create(lv_obj_t* parent);
    void update(uint16_t rpm, bool rpm_valid, uint32_t now_ms,
                const ShiftLightConfig& config);
private:
    std::array<lv_obj_t*, 12> segments_{};
};
