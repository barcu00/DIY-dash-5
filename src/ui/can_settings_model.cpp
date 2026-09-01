#include "can_settings_model.h"

#include <array>

namespace {
constexpr std::array<uint32_t, 4> kBitrates = {125000U, 250000U, 500000U, 1000000U};
}

uint32_t CanSettingsModel::nextBitrate(AppConfig& config) {
    std::size_t current = kBitrates.size() - 1U;
    for (std::size_t i = 0; i < kBitrates.size(); ++i) {
        if (config.can_bitrate == kBitrates[i]) {
            current = i;
            break;
        }
    }
    config.can_bitrate = kBitrates[(current + 1U) % kBitrates.size()];
    return config.can_bitrate;
}

DataSource CanSettingsModel::nextSource(AppConfig& config) {
    const uint8_t current = static_cast<uint8_t>(config.data_source);
    config.data_source = static_cast<DataSource>((current + 1U) % 3U);
    return config.data_source;
}

uint16_t CanSettingsModel::adjustBaseId(AppConfig& config, int32_t delta) {
    int32_t value = static_cast<int32_t>(config.ecumaster_base_id) + delta;
    if (value < 0) value = 0;
    if (value > 0x7F8) value = 0x7F8;
    config.ecumaster_base_id = static_cast<uint16_t>(value);
    return config.ecumaster_base_id;
}

uint32_t CanSettingsModel::adjustTimeout(AppConfig& config, int32_t delta_ms) {
    int32_t value = static_cast<int32_t>(config.can_timeout_ms) + delta_ms;
    if (value < 50) value = 50;
    if (value > 10000) value = 10000;
    config.can_timeout_ms = static_cast<uint32_t>(value);
    return config.can_timeout_ms;
}
