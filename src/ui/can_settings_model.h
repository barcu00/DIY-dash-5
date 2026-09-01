#pragma once

#include <cstdint>

#include "settings/app_config.h"

class CanSettingsModel {
public:
    static uint32_t nextBitrate(AppConfig& config);
    static DataSource nextSource(AppConfig& config);
    static uint16_t adjustBaseId(AppConfig& config, int32_t delta);
    static uint32_t adjustTimeout(AppConfig& config, int32_t delta_ms);
};
