#pragma once

#include <cstddef>
#include <cstdint>

enum class ParameterId : uint8_t {
    Rpm,
    Map,
    Lambda,
    Tps,
    Clt,
    Iat,
    OilPressure,
    OilTemperature,
    BatteryVoltage,
    Speed,
    Gear,
    FuelPressure,
    Count,
};

using VehicleSignal = ParameterId;

constexpr std::size_t parameterCount() {
    return static_cast<std::size_t>(ParameterId::Count);
}

enum class NativeUnit : uint8_t {
    None,
    Rpm,
    Bar,
    Lambda,
    Percent,
    Celsius,
    Volt,
    Kph,
    Gear,
};
