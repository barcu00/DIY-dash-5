#pragma once

#include <cstdint>

#include "settings/app_config.h"
#include "telemetry/vehicle_state.h"

enum class FlagTileState : uint8_t {
    Unavailable,
    Off,
    On,
};

struct FlagTilePresentation {
    FlagTileState state = FlagTileState::Unavailable;
    const char* status_text = "UNAVAILABLE";
    uint32_t rail_rgb = 0x27313AU;
    uint32_t background_rgb = 0x111820U;
    uint32_t pill_rgb = 0x737E87U;
    bool active_tint = false;
};

FlagTilePresentation flagTilePresentation(const SignalValue& signal,
                                          FlagActiveColor color);
