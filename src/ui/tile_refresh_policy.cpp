#include "tile_refresh_policy.h"

uint32_t tileRefreshIntervalMs(ParameterId parameter) {
    switch (parameter) {
        case ParameterId::Rpm:
        case ParameterId::Map:
        case ParameterId::Tps:
        case ParameterId::Speed:
        case ParameterId::BoostTarget:
        case ParameterId::InjectorPulseWidth:
        case ParameterId::AcceleratorPosition:
        case ParameterId::MassAirFlow:
        case ParameterId::WheelSpeedLf:
        case ParameterId::WheelSpeedLr:
        case ParameterId::WheelSpeedRf:
        case ParameterId::WheelSpeedRr:
            return 25U;

        case ParameterId::Lambda:
        case ParameterId::OilPressure:
        case ParameterId::Gear:
        case ParameterId::FuelPressure:
        case ParameterId::BarometricPressure:
        case ParameterId::CoolantPressure:
        case ParameterId::Lambda2:
        case ParameterId::IgnitionTiming:
        case ParameterId::InjectorDuty:
            return 50U;

        case ParameterId::Clt:
        case ParameterId::Iat:
        case ParameterId::OilTemperature:
        case ParameterId::BatteryVoltage:
        case ParameterId::FuelTemperature:
        case ParameterId::EthanolContent:
        case ParameterId::Egt1:
        case ParameterId::Egt2:
        case ParameterId::Egt3:
        case ParameterId::Egt4:
        case ParameterId::Egt5:
        case ParameterId::Egt6:
        case ParameterId::Egt7:
        case ParameterId::Egt8:
        case ParameterId::Count:
            return 100U;
    }
    return 100U;
}
