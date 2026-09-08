#include "app_config.h"

#include <algorithm>
#include <cmath>
#include <cstring>

#include "telemetry/parameter_registry.h"

namespace {
constexpr uint32_t kDefaultCanBitrate = 500000U;
constexpr uint32_t kMinimumCanTimeoutMs = 100U;
constexpr uint32_t kMaximumCanTimeoutMs = 5000U;
constexpr uint16_t kMaximumWarningDelayMs = 10000U;
constexpr float kMaximumTileSetting = 999.0f;
constexpr uint16_t kMinimumShiftRpm = 0U;
constexpr uint16_t kMaximumShiftRpm = 10000U;
constexpr uint16_t kShiftRpmStep = 100U;
constexpr float kDefaultStoichAfr = 14.7f;

bool supportedBitrate(uint32_t bitrate) {
    return bitrate == 125000U || bitrate == 250000U ||
           bitrate == 500000U || bitrate == 1000000U;
}

bool validParameter(ParameterId parameter) {
    return static_cast<std::size_t>(parameter) < parameterCount();
}

TileConfig tile(ParameterId parameter) {
    TileConfig config;
    config.parameter = parameter;
    config.decimals = parameterDescriptor(parameter).default_decimals;
    config.temperature_bar = defaultTemperatureBarConfig(parameter);
    return config;
}

void setProfileId(CanSettings& settings, const char* value) {
    settings.profile_id.fill('\0');
    std::strncpy(settings.profile_id.data(), value,
                 settings.profile_id.size() - 1U);
}

void validateUnits(UnitSettings& units) {
    if (static_cast<uint8_t>(units.temperature) >
        static_cast<uint8_t>(TemperatureUnit::Fahrenheit)) {
        units.temperature = TemperatureUnit::Celsius;
    }
    if (static_cast<uint8_t>(units.pressure) >
        static_cast<uint8_t>(PressureUnit::Psi)) {
        units.pressure = PressureUnit::Bar;
    }
    if (static_cast<uint8_t>(units.speed) >
        static_cast<uint8_t>(SpeedUnit::Mph)) {
        units.speed = SpeedUnit::Kph;
    }
    if (static_cast<uint8_t>(units.mixture) >
        static_cast<uint8_t>(MixtureUnit::Afr)) {
        units.mixture = MixtureUnit::Lambda;
    }
    if (!std::isfinite(units.stoich_afr) || units.stoich_afr < 5.0f ||
        units.stoich_afr > 20.0f) {
        units.stoich_afr = kDefaultStoichAfr;
    }
}

template <std::size_t Count>
void validateTiles(std::array<TileConfig, Count>& tiles) {
    for (TileConfig& config : tiles) {
        if (!validParameter(config.parameter)) {
            config.parameter = ParameterId::Rpm;
        }
        config.decimals = std::min<uint8_t>(config.decimals, 3U);
        if (static_cast<uint8_t>(config.warning.direction) >
            static_cast<uint8_t>(WarningDirection::Below)) {
            config.warning.direction = WarningDirection::Above;
        }
        if (!std::isfinite(config.warning.threshold_native)) {
            config.warning.threshold_native = 0.0f;
        }
        config.warning.threshold_native = std::clamp(
            config.warning.threshold_native, 0.0f, kMaximumTileSetting);
        if (!std::isfinite(config.warning.hysteresis_native) ||
            config.warning.hysteresis_native < 0.0f) {
            config.warning.hysteresis_native = 0.0f;
        }
        config.warning.hysteresis_native = std::clamp(
            config.warning.hysteresis_native, 0.0f, kMaximumTileSetting);
        config.warning.delay_ms = std::min<uint16_t>(
            config.warning.delay_ms, kMaximumWarningDelayMs);

        TemperatureBarConfig& bar = config.temperature_bar;
        const bool valid_bar =
            std::isfinite(bar.minimum_native) &&
            std::isfinite(bar.ready_native) &&
            std::isfinite(bar.maximum_native) &&
            bar.minimum_native >= -kMaximumTileSetting &&
            bar.maximum_native <= kMaximumTileSetting &&
            bar.minimum_native < bar.ready_native &&
            bar.ready_native < bar.maximum_native;
        if (!valid_bar) {
            bar = defaultTemperatureBarConfig(config.parameter);
            bar.enabled = false;
        }
        if (parameterDescriptor(config.parameter).native_unit !=
            NativeUnit::Celsius) {
            bar.enabled = false;
        }
    }
}
}  // namespace

TemperatureBarConfig defaultTemperatureBarConfig(ParameterId parameter) {
    TemperatureBarConfig config;
    switch (parameter) {
        case ParameterId::Clt:
        case ParameterId::OilTemperature:
            config = {true, 40.0f, 75.0f, 130.0f};
            break;
        case ParameterId::Iat:
            config = {false, 0.0f, 40.0f, 80.0f};
            break;
        case ParameterId::FuelTemperature:
            config = {false, 0.0f, 40.0f, 100.0f};
            break;
        case ParameterId::Egt1:
        case ParameterId::Egt2:
        case ParameterId::Egt3:
        case ParameterId::Egt4:
        case ParameterId::Egt5:
        case ParameterId::Egt6:
        case ParameterId::Egt7:
        case ParameterId::Egt8:
            config = {false, 200.0f, 650.0f, 950.0f};
            break;
        default:
            config = {false, 0.0f, 1.0f, 2.0f};
            break;
    }
    return config;
}

AppConfig AppConfig::defaults() {
    AppConfig config;
    setProfileId(config.can, "none");

    config.dash_tiles = {{
        tile(ParameterId::Speed),
        tile(ParameterId::Map),
        tile(ParameterId::Lambda),
        tile(ParameterId::Clt),
        tile(ParameterId::Rpm),
        tile(ParameterId::Gear),
        tile(ParameterId::Tps),
        tile(ParameterId::BatteryVoltage),
        tile(ParameterId::FuelPressure),
        tile(ParameterId::OilPressure),
        tile(ParameterId::Iat),
        tile(ParameterId::OilPressure),
        tile(ParameterId::OilTemperature),
        tile(ParameterId::FuelPressure),
    }};

    config.track_tiles = {{
        tile(ParameterId::Speed),
        tile(ParameterId::Lambda),
        tile(ParameterId::Clt),
        tile(ParameterId::OilPressure),
        tile(ParameterId::Rpm),
        tile(ParameterId::Gear),
        tile(ParameterId::Speed),
        tile(ParameterId::Map),
        tile(ParameterId::Tps),
        tile(ParameterId::Iat),
        tile(ParameterId::OilTemperature),
        tile(ParameterId::BatteryVoltage),
    }};
    return config;
}

ValidationResult AppConfig::validate() {
    ValidationResult result;

    if (data_source != DataSource::Can && data_source != DataSource::Demo) {
        data_source = DataSource::Demo;
    }
    brightness_percent =
        std::clamp<uint8_t>(brightness_percent, 20U, 100U);
    if (!supportedBitrate(can.bitrate)) {
        can.bitrate = kDefaultCanBitrate;
    }
    can.timeout_ms = std::clamp<uint32_t>(
        can.timeout_ms, kMinimumCanTimeoutMs, kMaximumCanTimeoutMs);
    can.profile_id.back() = '\0';
    if (can.profile_id.front() == '\0') {
        setProfileId(can, "none");
    }

    validateUnits(units);
    validateTiles(dash_tiles);
    validateTiles(track_tiles);

    const bool shift_in_range = shift.start_rpm >= kMinimumShiftRpm &&
                                shift.max_rpm <= kMaximumShiftRpm;
    const bool shift_on_steps =
        shift.start_rpm % kShiftRpmStep == 0U &&
        shift.red_rpm % kShiftRpmStep == 0U &&
        shift.flash_rpm % kShiftRpmStep == 0U &&
        shift.max_rpm % kShiftRpmStep == 0U;
    result.shift_order_valid =
        shift_in_range && shift_on_steps &&
        shift.red_rpm >= shift.start_rpm + kShiftRpmStep &&
        shift.flash_rpm >= shift.red_rpm + kShiftRpmStep &&
        shift.flash_rpm <= shift.max_rpm;
    result.valid = result.shift_order_valid;
    return result;
}
