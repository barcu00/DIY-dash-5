#pragma once

#include <array>
#include <cstdint>
#include <lvgl.h>

#include "settings/app_config.h"

enum class StatusTone : uint8_t {
    Normal,
    Info,
    Caution,
    Alert,
};

// Status band of the OEM SPORT and ENDURANCE layouts: one line of state
// (warnings, warm-up, RaceChrono) with a short explanation below.
class StatusBandView {
public:
    void create(lv_obj_t* parent);
    void apply(DashboardLayout layout);
    bool visible() const;
    void show(StatusTone tone, const char* title, const char* detail);

private:
    lv_obj_t* root_ = nullptr;
    lv_obj_t* dot_ = nullptr;
    lv_obj_t* title_ = nullptr;
    lv_obj_t* detail_ = nullptr;
    StatusTone tone_ = StatusTone::Normal;
    bool initialized_ = false;
    std::array<char, 96> last_title_{};
    std::array<char, 128> last_detail_{};
};
