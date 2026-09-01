#include "parameter_registry.h"

#include <cstddef>

namespace {
constexpr IconId kNoIcon = static_cast<IconId>(0);

constexpr ParameterDescriptor kDescriptors[] = {
    {ParameterId::Rpm, "Engine Speed", "RPM", "rpm", 0, 0, 10000, false, kNoIcon},
    {ParameterId::Tps, "Throttle Position", "TPS", "%", 1, 0, 100, false, kNoIcon},
    {ParameterId::Map, "Manifold Pressure", "MAP", "bar", 2, 0, 5, false, kNoIcon},
    {ParameterId::BoostTarget, "Boost Target", "BOOST TGT", "bar", 2, 0, 5, false, kNoIcon},
    {ParameterId::InjectorPulseWidth, "Injector Pulse Width", "INJ PW", "ms", 2, 0, 30, false, kNoIcon},
    {ParameterId::IgnitionAngle, "Ignition Angle", "IGN", "deg", 1, -20, 60, false, kNoIcon},
    {ParameterId::Dwell, "Ignition Dwell", "DWELL", "ms", 2, 0, 10, false, kNoIcon},
    {ParameterId::Iat, "Intake Air Temperature", "IAT", "C", 0, -50, 200, false, kNoIcon},
    {ParameterId::Clt, "Coolant Temperature", "CLT", "C", 0, -50, 200, false, kNoIcon},
    {ParameterId::OilTemperature, "Oil Temperature", "OIL TEMP", "C", 0, -50, 200, false, kNoIcon},
    {ParameterId::EcuTemperature, "ECU Temperature", "ECU TEMP", "C", 0, -50, 150, false, kNoIcon},
    {ParameterId::Egt1, "Exhaust Gas Temperature 1", "EGT1", "C", 0, 0, 1200, false, kNoIcon},
    {ParameterId::Egt2, "Exhaust Gas Temperature 2", "EGT2", "C", 0, 0, 1200, false, kNoIcon},
    {ParameterId::BarometricPressure, "Barometric Pressure", "BARO", "bar", 2, 0, 2, false, kNoIcon},
    {ParameterId::OilPressure, "Oil Pressure", "OIL PRESS", "bar", 1, 0, 10, false, kNoIcon},
    {ParameterId::FuelPressure, "Fuel Pressure", "FUEL PRESS", "bar", 1, 0, 20, false, kNoIcon},
    {ParameterId::Lambda, "Lambda", "LAMBDA", "lambda", 2, 0, 2, false, kNoIcon},
    {ParameterId::LambdaTarget, "Lambda Target", "LAM TGT", "lambda", 2, 0, 2, false, kNoIcon},
    {ParameterId::LambdaCorrection, "Lambda Correction", "LAM CORR", "%", 1, -100, 100, false, kNoIcon},
    {ParameterId::EthanolContent, "Ethanol Content", "ETHANOL", "%", 1, 0, 100, false, kNoIcon},
    {ParameterId::FuelUsed, "Fuel Used", "FUEL USED", "L", 2, 0, 9999, false, kNoIcon},
    {ParameterId::VehicleSpeed, "Vehicle Speed", "SPEED", "km/h", 0, 0, 400, false, kNoIcon},
    {ParameterId::Gear, "Gear", "GEAR", "", 0, -1, 10, false, kNoIcon},
    {ParameterId::DbwPosition, "DBW Position", "DBW POS", "%", 1, 0, 100, false, kNoIcon},
    {ParameterId::DbwTarget, "DBW Target", "DBW TGT", "%", 1, 0, 100, false, kNoIcon},
    {ParameterId::BatteryVoltage, "Battery Voltage", "BATTERY", "V", 1, 0, 20, false, kNoIcon},
    {ParameterId::Ain1, "Analog Input 1", "AIN1", "V", 2, 0, 5, false, kNoIcon},
    {ParameterId::Ain2, "Analog Input 2", "AIN2", "V", 2, 0, 5, false, kNoIcon},
    {ParameterId::Ain3, "Analog Input 3", "AIN3", "V", 2, 0, 5, false, kNoIcon},
    {ParameterId::Ain4, "Analog Input 4", "AIN4", "V", 2, 0, 5, false, kNoIcon},
    {ParameterId::Ain5, "Analog Input 5", "AIN5", "V", 2, 0, 5, false, kNoIcon},
    {ParameterId::Ain6, "Analog Input 6", "AIN6", "V", 2, 0, 5, false, kNoIcon},
    {ParameterId::Pwm1, "PWM 1", "PWM1", "%", 1, 0, 100, false, kNoIcon},
    {ParameterId::Pwm2, "PWM 2", "PWM2", "%", 1, 0, 100, false, kNoIcon},
    {ParameterId::TcDifferentialRpmRaw, "TC Differential RPM Raw", "TC DRPM", "rpm", 0, -10000, 10000, false, kNoIcon},
    {ParameterId::TcDifferentialRpmFiltered, "TC Differential RPM Filtered", "TC FILT", "rpm", 0, -10000, 10000, false, kNoIcon},
    {ParameterId::TcTorqueReduction, "TC Torque Reduction", "TC RED", "%", 1, 0, 100, false, kNoIcon},
    {ParameterId::PitLimiterTorqueReduction, "Pit Limiter Torque Reduction", "PIT RED", "%", 1, 0, 100, false, kNoIcon},
    {ParameterId::EcuError, "ECU Error", "ECU ERR", "", 0, 0, 1, true, kNoIcon},
    {ParameterId::LaunchControlActive, "Launch Control", "LC", "", 0, 0, 1, true, kNoIcon},
    {ParameterId::AntiLagActive, "Anti Lag", "ALS", "", 0, 0, 1, true, kNoIcon},
    {ParameterId::GearCutActive, "Gear Cut", "GEAR CUT", "", 0, 0, 1, true, kNoIcon},
    {ParameterId::TractionControlActive, "Traction Control", "TC", "", 0, 0, 1, true, kNoIcon},
    {ParameterId::PitLimiterActive, "Pit Limiter", "PIT", "", 0, 0, 1, true, kNoIcon},
    {ParameterId::BrakeActive, "Brake Input", "BRAKE", "", 0, 0, 1, true, kNoIcon},
    {ParameterId::FuelPumpActive, "Fuel Pump", "FUEL PUMP", "", 0, 0, 1, true, kNoIcon},
    {ParameterId::FanActive, "Fan", "FAN", "", 0, 0, 1, true, kNoIcon},
    {ParameterId::NitrousActive, "Nitrous", "NITROUS", "", 0, 0, 1, true, kNoIcon},
    {ParameterId::CanSwitch1, "CAN Switch 1", "CAN SW1", "", 0, 0, 1, true, kNoIcon},
    {ParameterId::CanSwitch2, "CAN Switch 2", "CAN SW2", "", 0, 0, 1, true, kNoIcon},
    {ParameterId::CanSwitch3, "CAN Switch 3", "CAN SW3", "", 0, 0, 1, true, kNoIcon},
    {ParameterId::CanSwitch4, "CAN Switch 4", "CAN SW4", "", 0, 0, 1, true, kNoIcon},
};

static_assert(sizeof(kDescriptors) / sizeof(kDescriptors[0]) == ParameterRegistry::count(),
              "Parameter descriptor table must cover every ParameterId");

constexpr std::size_t idx(ParameterId id) {
    return static_cast<std::size_t>(parameterIndex(id));
}
}

ParameterRegistry::ParameterRegistry() {
    picker_visible_.fill(true);
}

const ParameterDescriptor& ParameterRegistry::descriptor(ParameterId id) const {
    const auto index = idx(id);
    if (index >= count()) {
        return kDescriptors[0];
    }
    return kDescriptors[index];
}

const ParameterValue& ParameterRegistry::value(ParameterId id) const {
    const auto index = idx(id);
    if (index >= count()) {
        return values_[0];
    }
    return values_[index];
}

void ParameterRegistry::set(ParameterId id, float new_value, uint32_t now_ms) {
    const auto index = idx(id);
    if (index >= count()) {
        return;
    }
    values_[index].value = new_value;
    values_[index].valid = true;
    values_[index].updated_ms = now_ms;
}

void ParameterRegistry::invalidateAll() {
    for (auto& value : values_) {
        value.valid = false;
    }
}

void ParameterRegistry::setPickerVisible(ParameterId id, bool visible) {
    const auto index = idx(id);
    if (index >= count()) {
        return;
    }
    picker_visible_[index] = visible;
}

bool ParameterRegistry::pickerVisible(ParameterId id) const {
    const auto index = idx(id);
    return index < count() ? picker_visible_[index] : false;
}
