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
    {ParameterId::BarometricPressure, "Barometric pressure", "BARO", NativeUnit::Bar, 2U},
    {ParameterId::BoostTarget, "Boost target", "BOOST TARGET", NativeUnit::Bar, 2U},
    {ParameterId::CoolantPressure, "Coolant pressure", "CLT PRESS", NativeUnit::Bar, 1U},
    {ParameterId::FuelTemperature, "Fuel temperature", "FUEL TEMP", NativeUnit::Celsius, 0U},
    {ParameterId::EthanolContent, "Ethanol content", "ETHANOL", NativeUnit::Percent, 0U},
    {ParameterId::Lambda2, "Lambda 2", "LAMBDA 2", NativeUnit::Lambda, 2U},
    {ParameterId::IgnitionTiming, "Ignition timing", "IGN TIMING", NativeUnit::Degrees, 1U},
    {ParameterId::InjectorDuty, "Injector duty", "INJ DUTY", NativeUnit::Percent, 1U},
    {ParameterId::InjectorPulseWidth, "Injector pulse width", "INJ PW", NativeUnit::Milliseconds, 2U},
    {ParameterId::AcceleratorPosition, "Accelerator position", "ACCEL", NativeUnit::Percent, 1U},
    {ParameterId::MassAirFlow, "Mass air flow", "MAF", NativeUnit::GramsPerSecond, 1U},
    {ParameterId::Egt1, "Exhaust gas temperature 1", "EGT 1", NativeUnit::Celsius, 0U},
    {ParameterId::Egt2, "Exhaust gas temperature 2", "EGT 2", NativeUnit::Celsius, 0U},
    {ParameterId::Egt3, "Exhaust gas temperature 3", "EGT 3", NativeUnit::Celsius, 0U},
    {ParameterId::Egt4, "Exhaust gas temperature 4", "EGT 4", NativeUnit::Celsius, 0U},
    {ParameterId::Egt5, "Exhaust gas temperature 5", "EGT 5", NativeUnit::Celsius, 0U},
    {ParameterId::Egt6, "Exhaust gas temperature 6", "EGT 6", NativeUnit::Celsius, 0U},
    {ParameterId::Egt7, "Exhaust gas temperature 7", "EGT 7", NativeUnit::Celsius, 0U},
    {ParameterId::Egt8, "Exhaust gas temperature 8", "EGT 8", NativeUnit::Celsius, 0U},
    {ParameterId::WheelSpeedLf, "Wheel speed LF", "WHEEL LF", NativeUnit::Kph, 1U},
    {ParameterId::WheelSpeedLr, "Wheel speed LR", "WHEEL LR", NativeUnit::Kph, 1U},
    {ParameterId::WheelSpeedRf, "Wheel speed RF", "WHEEL RF", NativeUnit::Kph, 1U},
    {ParameterId::WheelSpeedRr, "Wheel speed RR", "WHEEL RR", NativeUnit::Kph, 1U},
}};

constexpr ParameterDescriptor kUnknown{
    ParameterId::Count, "Unknown", "---", NativeUnit::None, 0U};
}  // namespace

const ParameterDescriptor& parameterDescriptor(ParameterId id) {
    const std::size_t index = static_cast<std::size_t>(id);
    return index < kDescriptors.size() ? kDescriptors[index] : kUnknown;
}
