#pragma once

#include <cstdint>

#include "settings/app_config.h"

enum class TachColor : uint8_t {
    Normal,  // white, below YELLOW FROM
    Yellow,  // from YELLOW FROM (shift start)
    Red,     // from RED FROM
    Off,     // flash off-phase
};

struct TachBarState {
    uint16_t fill_permille = 0U;
    TachColor color = TachColor::Normal;
    bool flashing = false;
};

// The top tachometer bar doubles as the shift light: the whole filled part
// takes the colour of the highest threshold reached, and at FLASH the full
// bar blinks red at 4 Hz.
TachBarState tachBarState(float rpm, bool valid, uint16_t scale_max,
                          const ShiftLightConfig& shift, uint32_t now_ms);
inline bool operator==(const TachBarState& a, const TachBarState& b) {
    return a.fill_permille == b.fill_permille && a.color == b.color &&
           a.flashing == b.flashing;
}
inline bool operator!=(const TachBarState& a, const TachBarState& b) {
    return !(a == b);
}
