#include "ecumaster_decoder.h"

namespace {
constexpr float kKpaToBar = 0.01f;
constexpr float kAnalogScale = 5.0f / 1024.0f;
}

EcumasterDecoder::EcumasterDecoder(ParameterRegistry& registry) : registry_(registry) {}

void EcumasterDecoder::setBaseId(uint16_t base_id) {
    base_id_ = base_id;
}

uint16_t EcumasterDecoder::baseId() const {
    return base_id_;
}

uint16_t EcumasterDecoder::u16le(const uint8_t* data) {
    return static_cast<uint16_t>(data[0]) |
           (static_cast<uint16_t>(data[1]) << 8U);
}

int16_t EcumasterDecoder::s16le(const uint8_t* data) {
    return static_cast<int16_t>(u16le(data));
}

float EcumasterDecoder::bitValue(uint32_t value, uint8_t bit) {
    return (value & (1UL << bit)) != 0U ? 1.0f : 0.0f;
}

bool EcumasterDecoder::decode(uint32_t can_id, const uint8_t* data, std::size_t dlc, uint32_t now_ms) {
    if (data == nullptr || dlc != 8U || can_id < base_id_ || can_id > static_cast<uint32_t>(base_id_) + 7U) {
        return false;
    }

    switch (static_cast<uint16_t>(can_id - base_id_)) {
        case 0: decode600(data, now_ms); break;
        case 1: decode601(data, now_ms); break;
        case 2: decode602(data, now_ms); break;
        case 3: decode603(data, now_ms); break;
        case 4: decode604(data, now_ms); break;
        case 5: decode605(data, now_ms); break;
        case 6: decode606(data, now_ms); break;
        case 7: decode607(data, now_ms); break;
        default: return false;
    }
    return true;
}

void EcumasterDecoder::decode600(const uint8_t* d, uint32_t t) {
    registry_.set(ParameterId::Rpm, static_cast<float>(u16le(d)), t);
    registry_.set(ParameterId::Tps, static_cast<float>(d[2]) * 0.5f, t);
    registry_.set(ParameterId::Iat, static_cast<float>(static_cast<int8_t>(d[3])), t);
    registry_.set(ParameterId::Map, static_cast<float>(u16le(d + 4)) * kKpaToBar, t);
    registry_.set(ParameterId::InjectorPulseWidth, static_cast<float>(u16le(d + 6)) / 62.0f, t);
}

void EcumasterDecoder::decode601(const uint8_t* d, uint32_t t) {
    registry_.set(ParameterId::Ain1, static_cast<float>(u16le(d)) * kAnalogScale, t);
    registry_.set(ParameterId::Ain2, static_cast<float>(u16le(d + 2)) * kAnalogScale, t);
    registry_.set(ParameterId::Ain3, static_cast<float>(u16le(d + 4)) * kAnalogScale, t);
    registry_.set(ParameterId::Ain4, static_cast<float>(u16le(d + 6)) * kAnalogScale, t);
}

void EcumasterDecoder::decode602(const uint8_t* d, uint32_t t) {
    registry_.set(ParameterId::VehicleSpeed, static_cast<float>(u16le(d)), t);
    registry_.set(ParameterId::BarometricPressure, static_cast<float>(d[2]) * kKpaToBar, t);
    registry_.set(ParameterId::OilTemperature, static_cast<float>(d[3]), t);
    registry_.set(ParameterId::OilPressure, static_cast<float>(d[4]) / 16.0f, t);
    registry_.set(ParameterId::FuelPressure, static_cast<float>(d[5]) / 16.0f, t);
    registry_.set(ParameterId::Clt, static_cast<float>(s16le(d + 6)), t);
}

void EcumasterDecoder::decode603(const uint8_t* d, uint32_t t) {
    registry_.set(ParameterId::IgnitionAngle, static_cast<float>(static_cast<int8_t>(d[0])) * 0.5f, t);
    registry_.set(ParameterId::Dwell, static_cast<float>(d[1]) * 0.05f, t);
    registry_.set(ParameterId::Lambda, static_cast<float>(d[2]) / 128.0f, t);
    registry_.set(ParameterId::LambdaCorrection, static_cast<float>(d[3]) * 0.5f, t);
    registry_.set(ParameterId::Egt1, static_cast<float>(u16le(d + 4)), t);
    registry_.set(ParameterId::Egt2, static_cast<float>(u16le(d + 6)), t);
}

void EcumasterDecoder::decode604(const uint8_t* d, uint32_t t) {
    registry_.set(ParameterId::Gear, static_cast<float>(d[0]), t);
    registry_.set(ParameterId::EcuTemperature, static_cast<float>(static_cast<int8_t>(d[1])), t);
    registry_.set(ParameterId::BatteryVoltage, static_cast<float>(u16le(d + 2)) * 0.027f, t);

    const uint16_t errors = u16le(d + 4);
    registry_.set(ParameterId::EcuError, (errors & 0x07FFU) != 0U ? 1.0f : 0.0f, t);
    registry_.set(ParameterId::CltSensorError, bitValue(errors, 0), t);
    registry_.set(ParameterId::IatSensorError, bitValue(errors, 1), t);
    registry_.set(ParameterId::MapSensorError, bitValue(errors, 2), t);
    registry_.set(ParameterId::WidebandError, bitValue(errors, 3), t);
    registry_.set(ParameterId::Egt1SensorError, bitValue(errors, 4), t);
    registry_.set(ParameterId::Egt2SensorError, bitValue(errors, 5), t);
    registry_.set(ParameterId::EgtAlarm, bitValue(errors, 6), t);
    registry_.set(ParameterId::Knocking, bitValue(errors, 7), t);
    registry_.set(ParameterId::FlexFuelSensorError, bitValue(errors, 8), t);
    registry_.set(ParameterId::DbwError, bitValue(errors, 9), t);
    registry_.set(ParameterId::FuelPressureRelativeError, bitValue(errors, 10), t);

    const uint8_t flags = d[6];
    registry_.set(ParameterId::GearCutActive, bitValue(flags, 0), t);
    registry_.set(ParameterId::AntiLagActive, bitValue(flags, 1), t);
    registry_.set(ParameterId::LaunchControlActive, bitValue(flags, 2), t);
    registry_.set(ParameterId::IdleActive, bitValue(flags, 3), t);
    registry_.set(ParameterId::TableSet2Active, bitValue(flags, 4), t);
    registry_.set(ParameterId::TractionControlActive, bitValue(flags, 5), t);
    registry_.set(ParameterId::PitLimiterActive, bitValue(flags, 6), t);
    registry_.set(ParameterId::BrakeActive, bitValue(flags, 7), t);
    registry_.set(ParameterId::EthanolContent, static_cast<float>(d[7]), t);
}

void EcumasterDecoder::decode605(const uint8_t* d, uint32_t t) {
    registry_.set(ParameterId::DbwPosition, static_cast<float>(d[0]) * 0.5f, t);
    registry_.set(ParameterId::DbwTarget, static_cast<float>(d[1]) * 0.5f, t);
    registry_.set(ParameterId::TcDifferentialRpmRaw, static_cast<float>(s16le(d + 2)), t);
    registry_.set(ParameterId::TcDifferentialRpmFiltered, static_cast<float>(u16le(d + 4)), t);
    registry_.set(ParameterId::TcTorqueReduction, static_cast<float>(d[6]), t);
    registry_.set(ParameterId::PitLimiterTorqueReduction, static_cast<float>(d[7]), t);
}

void EcumasterDecoder::decode606(const uint8_t* d, uint32_t t) {
    registry_.set(ParameterId::Ain5, static_cast<float>(u16le(d)) * kAnalogScale, t);
    registry_.set(ParameterId::Ain6, static_cast<float>(u16le(d + 2)) * kAnalogScale, t);

    const uint8_t out1 = d[4];
    registry_.set(ParameterId::ParametricOutput1, bitValue(out1, 0), t);
    registry_.set(ParameterId::ParametricOutput2, bitValue(out1, 1), t);
    registry_.set(ParameterId::ParametricOutput3, bitValue(out1, 2), t);
    registry_.set(ParameterId::ParametricOutput4, bitValue(out1, 3), t);
    registry_.set(ParameterId::ParametricOutput5, bitValue(out1, 4), t);
    registry_.set(ParameterId::VirtualOutput1, bitValue(out1, 5), t);
    registry_.set(ParameterId::VirtualOutput2, bitValue(out1, 6), t);
    registry_.set(ParameterId::VirtualOutput3, bitValue(out1, 7), t);

    const uint8_t out2 = d[5];
    registry_.set(ParameterId::CanSwitch1, bitValue(out2, 0), t);
    registry_.set(ParameterId::CanSwitch2, bitValue(out2, 1), t);
    registry_.set(ParameterId::CanSwitch3, bitValue(out2, 2), t);
    registry_.set(ParameterId::CanSwitch4, bitValue(out2, 3), t);
    registry_.set(ParameterId::CanSwitch5, bitValue(out2, 4), t);
    registry_.set(ParameterId::CanSwitch6, bitValue(out2, 5), t);
    registry_.set(ParameterId::CanSwitch7, bitValue(out2, 6), t);
    registry_.set(ParameterId::CanSwitch8, bitValue(out2, 7), t);

    const uint8_t out3 = d[6];
    registry_.set(ParameterId::Switch1, bitValue(out3, 0), t);
    registry_.set(ParameterId::Switch2, bitValue(out3, 1), t);
    registry_.set(ParameterId::Switch3, bitValue(out3, 2), t);
    registry_.set(ParameterId::MuxSwitch1, bitValue(out3, 3), t);
    registry_.set(ParameterId::MuxSwitch2, bitValue(out3, 4), t);
    registry_.set(ParameterId::MuxSwitch3, bitValue(out3, 5), t);
    registry_.set(ParameterId::LaunchMapSet, bitValue(out3, 6), t);
    registry_.set(ParameterId::AlsMapSet, bitValue(out3, 7), t);

    const uint8_t out4 = d[7];
    registry_.set(ParameterId::FuelPumpActive, bitValue(out4, 0), t);
    registry_.set(ParameterId::FanActive, bitValue(out4, 1), t);
    registry_.set(ParameterId::AcClutchActive, bitValue(out4, 2), t);
    registry_.set(ParameterId::AcFanActive, bitValue(out4, 3), t);
    registry_.set(ParameterId::NitrousActive, bitValue(out4, 4), t);
    registry_.set(ParameterId::StarterRequest, bitValue(out4, 5), t);
    registry_.set(ParameterId::BoostMapSet, bitValue(out4, 6), t);
}

void EcumasterDecoder::decode607(const uint8_t* d, uint32_t t) {
    registry_.set(ParameterId::BoostTarget, static_cast<float>(u16le(d)) * kKpaToBar, t);
    registry_.set(ParameterId::Pwm1, static_cast<float>(d[2]), t);
    registry_.set(ParameterId::DsgMode, static_cast<float>(d[3] & 0x0FU), t);
    registry_.set(ParameterId::LambdaTarget, static_cast<float>(d[4]) * 0.01f, t);
    registry_.set(ParameterId::Pwm2, static_cast<float>(d[5]), t);
    registry_.set(ParameterId::FuelUsed, static_cast<float>(u16le(d + 6)) * 0.01f, t);
}
