#pragma once

#include <cstdint>

#include "ui/icon_id.h"

struct IconAsset {
    const uint16_t* rows = nullptr;
    uint16_t row_count = 0;
    uint8_t width = 0;
    uint8_t height = 0;
};

class IconAssets {
public:
    static const IconAsset& get(IconId id);
};
