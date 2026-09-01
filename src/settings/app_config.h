#pragma once

#include <array>
#include <cstdint>

#include "alarms/warning_config.h"
#include "telemetry/data_source.h"
#include "telemetry/parameter_id.h"

enum class ValueFormatMode : uint8_t {
    Native = 0,
    Afr = 1,
};

struct TileConfig {
    ParameterId parameter = ParameterId::Rpm;
    bool visible = true;
    bool custom_label_enabled = false;
    std::array<char, 20> custom_label{};
    bool icon_enabled = true;
    uint16_t icon = 0U;
    uint8_t decimals = 1U;
    ValueFormatMode value_format = ValueFormatMode::Native;
};

struct AppConfig {
    static constexpr uint32_t kSchemaVersion = 2U;
    static constexpr uint8_t kTileCount = 12U;
    static constexpr uint16_t kParameterCount = parameterIndex(ParameterId::Count);

    uint32_t schema_version = kSchemaVersion;
    DataSource data_source = DataSource::Ecumaster;
    uint32_t can_bitrate = 1000000U;
    uint16_t ecumaster_base_id = 0x600U;
    uint32_t can_timeout_ms = 500U;

    ValueFormatMode lambda_format = ValueFormatMode::Native;
    float stoich_afr = 14.7f;

    // DASH layout. Kept as `tiles` for persisted v0.2 API compatibility.
    std::array<TileConfig, kTileCount> tiles{};
    std::array<TileConfig, kTileCount> track_tiles{};
    std::array<bool, kParameterCount> parameter_visible{};
    std::array<WarningConfig, kParameterCount> warnings{};

    static AppConfig defaults();
    static bool schemaCompatible(uint32_t version);

    bool validSchema() const;
    void validate();
    float lambdaToAfr(float lambda) const;
};
