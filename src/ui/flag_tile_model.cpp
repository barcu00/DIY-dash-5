#include "flag_tile_model.h"

namespace {
struct ActivePalette {
    uint32_t accent;
    uint32_t background;
};

ActivePalette palette(FlagActiveColor color) {
    switch (color) {
        case FlagActiveColor::Yellow:
            return {0xFFD166U, 0x262214U};
        case FlagActiveColor::Green:
            return {0x2FE38CU, 0x12271DU};
        case FlagActiveColor::Red:
            return {0xFF4D5AU, 0x2A1519U};
    }
    return {0xFFD166U, 0x262214U};
}
}  // namespace

FlagTilePresentation flagTilePresentation(const SignalValue& signal,
                                          FlagActiveColor color) {
    if (!signal.valid) {
        return {};
    }
    if (signal.value < 0.5f) {
        FlagTilePresentation result;
        result.state = FlagTileState::Off;
        result.status_text = "OFF";
        return result;
    }

    const ActivePalette active = palette(color);
    FlagTilePresentation result;
    result.state = FlagTileState::On;
    result.status_text = "ON";
    result.rail_rgb = active.accent;
    result.background_rgb = active.background;
    result.pill_rgb = active.accent;
    result.active_tint = true;
    return result;
}
