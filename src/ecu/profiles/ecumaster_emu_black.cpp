#include "ecu/profiles/profile_declarations.h"

namespace {
constexpr uint32_t kFastTimeout = 250U;

constexpr CanSignalDefinition flag8(ParameterId parameter,
                                    uint8_t byte_offset, uint8_t bit) {
    return {parameter, byte_offset, RawType::Unsigned8, ByteOrder::Little,
            1.0f, 0.0f, 0.0f, 1.0f, kFastTimeout,
            CanSignalKind::MaskedFlag, 1U << bit, bit, 1ULL << 1U};
}

constexpr CanSignalDefinition flag16(ParameterId parameter,
                                     uint8_t byte_offset, uint8_t bit) {
    return {parameter, byte_offset, RawType::Unsigned16, ByteOrder::Little,
            1.0f, 0.0f, 0.0f, 1.0f, kFastTimeout,
            CanSignalKind::MaskedFlag, 1U << bit, bit, 1ULL << 1U};
}

const CanSignalDefinition kFrame600[] = {
    {ParameterId::Rpm, 0U, RawType::Unsigned16, ByteOrder::Little, 1.0f, 0.0f,
     0.0f, 16000.0f, kFastTimeout},
    {ParameterId::Tps, 2U, RawType::Unsigned8, ByteOrder::Little, 0.5f, 0.0f,
     0.0f, 100.0f, kFastTimeout},
    {ParameterId::Iat, 3U, RawType::Signed8, ByteOrder::Little, 1.0f, 0.0f,
     -40.0f, 127.0f, kFastTimeout},
    {ParameterId::Map, 4U, RawType::Unsigned16, ByteOrder::Little, 0.01f, 0.0f,
     0.0f, 6.0f, kFastTimeout},
    {ParameterId::InjectorPulseWidth, 6U, RawType::Unsigned16,
     ByteOrder::Little, 0.016129032f, 0.0f, 0.0f, 100.0f, kFastTimeout},
};
const CanSignalDefinition kFrame602[] = {
    {ParameterId::Speed, 0U, RawType::Unsigned16, ByteOrder::Little, 1.0f, 0.0f,
     0.0f, 400.0f, kFastTimeout},
    {ParameterId::OilTemperature, 3U, RawType::Unsigned8, ByteOrder::Little,
     1.0f, 0.0f, 0.0f, 160.0f, kFastTimeout},
    {ParameterId::OilPressure, 4U, RawType::Unsigned8, ByteOrder::Little,
     0.0625f, 0.0f, 0.0f, 12.0f, kFastTimeout},
    {ParameterId::FuelPressure, 5U, RawType::Unsigned8, ByteOrder::Little,
     0.0625f, 0.0f, 0.0f, 12.0f, kFastTimeout},
    {ParameterId::Clt, 6U, RawType::Signed16, ByteOrder::Little, 1.0f, 0.0f,
     -40.0f, 250.0f, kFastTimeout},
};
const CanSignalDefinition kFrame603[] = {
    {ParameterId::IgnitionTiming, 0U, RawType::Signed8, ByteOrder::Little,
     0.5f, 0.0f, -100.0f, 100.0f, kFastTimeout},
    {ParameterId::Lambda, 2U, RawType::Unsigned8, ByteOrder::Little,
     0.0078125f, 0.0f, 0.0f, 2.0f, kFastTimeout},
    {ParameterId::Egt1, 4U, RawType::Unsigned16, ByteOrder::Little, 1.0f,
     0.0f, 0.0f, 1300.0f, kFastTimeout},
    {ParameterId::Egt2, 6U, RawType::Unsigned16, ByteOrder::Little, 1.0f,
     0.0f, 0.0f, 1300.0f, kFastTimeout},
};
const CanSignalDefinition kFrame604[] = {
    {ParameterId::Gear, 0U, RawType::Unsigned8, ByteOrder::Little, 1.0f, 0.0f,
     0.0f, 7.0f, kFastTimeout},
    {ParameterId::BatteryVoltage, 2U, RawType::Unsigned16, ByteOrder::Little,
     0.027f, 0.0f, 0.0f, 20.0f, kFastTimeout},
    {ParameterId::EthanolContent, 7U, RawType::Unsigned8, ByteOrder::Little,
     1.0f, 0.0f, 0.0f, 100.0f, kFastTimeout},
    flag16(ParameterId::CoolantSensorError, 4U, 0U),
    flag16(ParameterId::IntakeAirSensorError, 4U, 1U),
    flag16(ParameterId::MapSensorError, 4U, 2U),
    flag16(ParameterId::WidebandSensorError, 4U, 3U),
    flag16(ParameterId::Egt1SensorError, 4U, 4U),
    flag16(ParameterId::Egt2SensorError, 4U, 5U),
    flag16(ParameterId::EgtHighAlarm, 4U, 6U),
    flag16(ParameterId::KnockDetected, 4U, 7U),
    flag16(ParameterId::FlexFuelSensorError, 4U, 8U),
    flag16(ParameterId::DbwError, 4U, 9U),
    flag16(ParameterId::FuelPressureError, 4U, 10U),
    flag8(ParameterId::GearCutActive, 6U, 0U),
    flag8(ParameterId::AntiLagActive, 6U, 1U),
    flag8(ParameterId::LaunchControlActive, 6U, 2U),
    flag8(ParameterId::IdleActive, 6U, 3U),
    flag8(ParameterId::TractionControlActive, 6U, 5U),
    flag8(ParameterId::PitLimiterActive, 6U, 6U),
};
const CanSignalDefinition kFrame605[] = {
    {ParameterId::AcceleratorPosition, 0U, RawType::Unsigned8,
     ByteOrder::Little, 0.5f, 0.0f, 0.0f, 100.0f, kFastTimeout},
};
const CanSignalDefinition kFrame606[] = {
    flag8(ParameterId::FuelPumpActive, 7U, 0U),
    flag8(ParameterId::CoolantFanActive, 7U, 1U),
    flag8(ParameterId::AcClutchActive, 7U, 2U),
    flag8(ParameterId::AcFanActive, 7U, 3U),
    flag8(ParameterId::NitrousActive, 7U, 4U),
    flag8(ParameterId::StarterRequestActive, 7U, 5U),
};
const CanSignalDefinition kFrame607[] = {
    {ParameterId::BoostTarget, 0U, RawType::Unsigned16, ByteOrder::Little,
     0.01f, 0.0f, 0.0f, 6.0f, kFastTimeout},
};
const CanFrameDefinition kFrames[] = {
    {0x600U, false, 8U, 0U, 0U, 0U, kFrame600,
     sizeof(kFrame600) / sizeof(kFrame600[0])},
    {0x602U, false, 8U, 0U, 0U, 0U, kFrame602,
     sizeof(kFrame602) / sizeof(kFrame602[0])},
    {0x603U, false, 8U, 0U, 0U, 0U, kFrame603,
     sizeof(kFrame603) / sizeof(kFrame603[0])},
    {0x604U, false, 8U, 0U, 0U, 0U, kFrame604,
     sizeof(kFrame604) / sizeof(kFrame604[0])},
    {0x605U, false, 8U, 0U, 0U, 0U, kFrame605,
     sizeof(kFrame605) / sizeof(kFrame605[0])},
    {0x606U, false, 8U, 0U, 0U, 0U, kFrame606,
     sizeof(kFrame606) / sizeof(kFrame606[0])},
    {0x607U, false, 8U, 0U, 0U, 0U, kFrame607,
     sizeof(kFrame607) / sizeof(kFrame607[0])},
};
}  // namespace

const CanProfile kEcumasterEmuBlackProfile{
    "ecumaster_emu_black", "ECUMaster EMU Black", "ECUMaster EMU Black",
    "CAN Stream 1.4", CanProfileVerification::Verified, 1000000U,
    "https://www.ecumaster.com/files/EMU_BLACK/EMU_BLACK_manual.pdf",
    "document 1.4; firmware 2.169+; 2026-07-09",
    "Default 0x600 base, documented little-endian stream", kFrames,
    sizeof(kFrames) / sizeof(kFrames[0])};
