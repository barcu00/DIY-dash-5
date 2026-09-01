#include "icon_catalog.h"

IconId IconCatalog::defaultIcon(ParameterId id) {
    switch (id) {
        case ParameterId::Rpm: return IconId::Rpm;
        case ParameterId::Tps: return IconId::Throttle;
        case ParameterId::Map:
        case ParameterId::BoostTarget:
        case ParameterId::BoostMapSet: return IconId::Boost;
        case ParameterId::InjectorPulseWidth: return IconId::Injector;
        case ParameterId::IgnitionAngle:
        case ParameterId::Dwell:
        case ParameterId::Knocking: return IconId::Ignition;
        case ParameterId::Iat:
        case ParameterId::Clt:
        case ParameterId::EcuTemperature:
        case ParameterId::Egt1:
        case ParameterId::Egt2: return IconId::Temperature;
        case ParameterId::OilTemperature: return IconId::OilTemperature;
        case ParameterId::BarometricPressure: return IconId::Pressure;
        case ParameterId::OilPressure: return IconId::OilPressure;
        case ParameterId::FuelPressure:
        case ParameterId::FuelPressureRelativeError: return IconId::FuelPressure;
        case ParameterId::Lambda:
        case ParameterId::LambdaTarget:
        case ParameterId::LambdaCorrection:
        case ParameterId::WidebandError: return IconId::Lambda;
        case ParameterId::EthanolContent:
        case ParameterId::FuelUsed:
        case ParameterId::FlexFuelSensorError:
        case ParameterId::FuelPumpActive: return IconId::Fuel;
        case ParameterId::VehicleSpeed: return IconId::Speed;
        case ParameterId::Gear:
        case ParameterId::GearCutActive:
        case ParameterId::DsgMode: return IconId::Gear;
        case ParameterId::DbwPosition:
        case ParameterId::DbwTarget:
        case ParameterId::DbwError: return IconId::Dbw;
        case ParameterId::BatteryVoltage: return IconId::Battery;
        case ParameterId::Ain1:
        case ParameterId::Ain2:
        case ParameterId::Ain3:
        case ParameterId::Ain4:
        case ParameterId::Ain5:
        case ParameterId::Ain6: return IconId::Analog;
        case ParameterId::Pwm1:
        case ParameterId::Pwm2: return IconId::Pwm;
        case ParameterId::TcDifferentialRpmRaw:
        case ParameterId::TcDifferentialRpmFiltered:
        case ParameterId::TcTorqueReduction:
        case ParameterId::TractionControlActive: return IconId::Traction;
        case ParameterId::PitLimiterTorqueReduction:
        case ParameterId::PitLimiterActive: return IconId::Limiter;
        case ParameterId::EcuError:
        case ParameterId::CltSensorError:
        case ParameterId::IatSensorError:
        case ParameterId::MapSensorError:
        case ParameterId::Egt1SensorError:
        case ParameterId::Egt2SensorError:
        case ParameterId::EgtAlarm: return IconId::Warning;
        case ParameterId::LaunchControlActive:
        case ParameterId::LaunchMapSet: return IconId::Launch;
        case ParameterId::AntiLagActive:
        case ParameterId::AlsMapSet: return IconId::AntiLag;
        case ParameterId::BrakeActive: return IconId::Brake;
        case ParameterId::FanActive:
        case ParameterId::AcFanActive: return IconId::Fan;
        case ParameterId::NitrousActive: return IconId::Nitrous;
        case ParameterId::CanSwitch1:
        case ParameterId::CanSwitch2:
        case ParameterId::CanSwitch3:
        case ParameterId::CanSwitch4:
        case ParameterId::CanSwitch5:
        case ParameterId::CanSwitch6:
        case ParameterId::CanSwitch7:
        case ParameterId::CanSwitch8: return IconId::Can;
        case ParameterId::IdleActive:
        case ParameterId::TableSet2Active:
        case ParameterId::Switch1:
        case ParameterId::Switch2:
        case ParameterId::Switch3:
        case ParameterId::MuxSwitch1:
        case ParameterId::MuxSwitch2:
        case ParameterId::MuxSwitch3: return IconId::Switch;
        case ParameterId::ParametricOutput1:
        case ParameterId::ParametricOutput2:
        case ParameterId::ParametricOutput3:
        case ParameterId::ParametricOutput4:
        case ParameterId::ParametricOutput5:
        case ParameterId::VirtualOutput1:
        case ParameterId::VirtualOutput2:
        case ParameterId::VirtualOutput3: return IconId::Output;
        case ParameterId::AcClutchActive: return IconId::AirConditioning;
        case ParameterId::StarterRequest: return IconId::Starter;
        case ParameterId::Count: return IconId::Invalid;
    }
    return IconId::Generic;
}

const char* IconCatalog::name(IconId id) {
    switch (id) {
        case IconId::Invalid: return "";
        case IconId::Generic: return "GENERIC";
        case IconId::Rpm: return "RPM";
        case IconId::Speed: return "SPEED";
        case IconId::Gear: return "GEAR";
        case IconId::Throttle: return "THROTTLE";
        case IconId::Boost: return "BOOST";
        case IconId::Pressure: return "PRESSURE";
        case IconId::Temperature: return "TEMPERATURE";
        case IconId::OilPressure: return "OIL PRESSURE";
        case IconId::OilTemperature: return "OIL TEMPERATURE";
        case IconId::FuelPressure: return "FUEL PRESSURE";
        case IconId::Fuel: return "FUEL";
        case IconId::Lambda: return "LAMBDA / AFR";
        case IconId::Injector: return "INJECTOR";
        case IconId::Ignition: return "IGNITION";
        case IconId::Battery: return "BATTERY";
        case IconId::Ecu: return "ECU";
        case IconId::Analog: return "ANALOG";
        case IconId::Pwm: return "PWM";
        case IconId::Dbw: return "DBW";
        case IconId::Traction: return "TRACTION";
        case IconId::Launch: return "LAUNCH";
        case IconId::AntiLag: return "ANTI-LAG";
        case IconId::Limiter: return "LIMITER";
        case IconId::Brake: return "BRAKE";
        case IconId::Fan: return "FAN";
        case IconId::Nitrous: return "NITROUS";
        case IconId::Can: return "CAN";
        case IconId::Warning: return "WARNING";
        case IconId::Switch: return "SWITCH";
        case IconId::Output: return "OUTPUT";
        case IconId::Starter: return "STARTER";
        case IconId::AirConditioning: return "A/C";
        case IconId::LapTime: return "LAP / TIME";
    }
    return "";
}
