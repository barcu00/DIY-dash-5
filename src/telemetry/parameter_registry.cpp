#include "parameter_registry.h"

#include <array>

namespace {
constexpr std::array<ParameterDescriptor, parameterCount()> kDescriptors{{
    {ParameterId::Rpm, "Engine speed", "RPM", NativeUnit::Rpm, 0U},
    {ParameterId::Map, "Manifold pressure", "MAP", NativeUnit::Bar, 2U},
    {ParameterId::Lambda, "Lambda", "LAMBDA", NativeUnit::Lambda, 2U},
    {ParameterId::Tps, "Throttle position", "TPS", NativeUnit::Percent, 0U},
    {ParameterId::Clt, "Coolant temperature", "CLT", NativeUnit::Celsius, 0U},
    {ParameterId::Iat, "Intake air temperature", "IAT", NativeUnit::Celsius, 0U},
    {ParameterId::OilPressure, "Oil pressure", "OIL PRESS", NativeUnit::Bar, 1U},
    {ParameterId::OilTemperature, "Oil temperature", "OIL TEMP", NativeUnit::Celsius, 0U},
    {ParameterId::BatteryVoltage, "Battery voltage", "BATTERY", NativeUnit::Volt, 1U},
    {ParameterId::Speed, "Vehicle speed", "SPEED", NativeUnit::Kph, 0U},
    {ParameterId::Gear, "Gear", "GEAR", NativeUnit::Gear, 0U},
    {ParameterId::FuelPressure, "Fuel pressure", "FUEL PRESS", NativeUnit::Bar, 1U},
}};

constexpr ParameterDescriptor kUnknown{
    ParameterId::Count, "Unknown", "---", NativeUnit::None, 0U};
}  // namespace

const ParameterDescriptor& parameterDescriptor(ParameterId id) {
    const std::size_t index = static_cast<std::size_t>(id);
    return index < kDescriptors.size() ? kDescriptors[index] : kUnknown;
}
