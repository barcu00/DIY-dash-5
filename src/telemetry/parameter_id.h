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
    BarometricPressure,
    BoostTarget,
    CoolantPressure,
    FuelTemperature,
    EthanolContent,
    Lambda2,
    IgnitionTiming,
    InjectorDuty,
    InjectorPulseWidth,
    AcceleratorPosition,
    MassAirFlow,
    Egt1,
    Egt2,
    Egt3,
    Egt4,
    Egt5,
    Egt6,
    Egt7,
    Egt8,
    WheelSpeedLf,
    WheelSpeedLr,
    WheelSpeedRf,
    WheelSpeedRr,
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
    Degrees,
    Milliseconds,
    GramsPerSecond,
};
