#pragma once

#include <cstdint>

#include "settings/app_config.h"

enum class TileOpenGesture : uint8_t {
    LongPress,
};

struct TileViewPolicy {
    bool value_centered = true;
    uint32_t value_rgb = 0xF2F5F7U;
    TileOpenGesture open_gesture = TileOpenGesture::LongPress;
    uint16_t long_press_ms = 600U;
};

TileViewPolicy tileViewPolicy(TileSize size);
