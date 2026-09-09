#include "ecu/profiles/profile_declarations.h"

namespace {
constexpr uint32_t kTimeout = 300U;

constexpr CanSignalDefinition flag(ParameterId parameter, uint8_t byte_offset,
                                   uint8_t bit) {
    return {parameter, byte_offset, RawType::Unsigned8, ByteOrder::Little,
            1.0f, 0.0f, 0.0f, 1.0f, kTimeout,
            CanSignalKind::MaskedFlag, 1U << bit, bit, 1ULL << 1U};
}
const CanSignalDefinition kStatus[] = {
    {ParameterId::Gear, 5U, RawType::Unsigned8, ByteOrder::Little, 1.0f, 0.0f,
     0.0f, 20.0f, kTimeout},
    flag(ParameterId::RevLimiterActive, 4U, 0U),
    flag(ParameterId::MainRelayActive, 4U, 1U),
    flag(ParameterId::FuelPumpActive, 4U, 2U),
    flag(ParameterId::CheckEngine, 4U, 3U),
    flag(ParameterId::O2HeaterActive, 4U, 4U),
    flag(ParameterId::LambdaProtectionActive, 4U, 5U),
    flag(ParameterId::CoolantFanActive, 4U, 6U),
    flag(ParameterId::CoolantFan2Active, 4U, 7U),
};
const CanSignalDefinition kStatus11[] = {
    flag(ParameterId::BrakePressed, 0U, 0U),
};
const CanSignalDefinition kSpeeds[] = {
    {ParameterId::Rpm, 0U, RawType::Unsigned16, ByteOrder::Little, 1.0f, 0.0f,
     0.0f, 30000.0f, kTimeout},
    {ParameterId::Speed, 6U, RawType::Unsigned8, ByteOrder::Little, 1.0f, 0.0f,
     0.0f, 255.0f, kTimeout},
    {ParameterId::IgnitionTiming, 2U, RawType::Signed16, ByteOrder::Little,
     0.02f, 0.0f, -100.0f, 100.0f, kTimeout},
    {ParameterId::InjectorDuty, 4U, RawType::Unsigned8, ByteOrder::Little,
     0.5f, 0.0f, 0.0f, 127.5f, kTimeout},
    {ParameterId::EthanolContent, 7U, RawType::Unsigned8, ByteOrder::Little,
     1.0f, 0.0f, 0.0f, 100.0f, kTimeout},
};
const CanSignalDefinition kThrottle[] = {
    {ParameterId::AcceleratorPosition, 0U, RawType::Signed16,
     ByteOrder::Little, 0.01f, 0.0f, 0.0f, 100.0f, kTimeout},
    {ParameterId::Tps, 2U, RawType::Signed16, ByteOrder::Little, 0.01f, 0.0f,
     0.0f, 100.0f, kTimeout},
};
const CanSignalDefinition kSensors1[] = {
    {ParameterId::Map, 0U, RawType::Unsigned16, ByteOrder::Little,
     0.0003333333f, 0.0f, 0.0f, 10.0f, kTimeout},
    {ParameterId::Clt, 2U, RawType::Unsigned8, ByteOrder::Little, 1.0f, -40.0f,
     -40.0f, 200.0f, kTimeout},
    {ParameterId::Iat, 3U, RawType::Unsigned8, ByteOrder::Little, 1.0f, -40.0f,
     -40.0f, 200.0f, kTimeout},
};
const CanSignalDefinition kSensors2[] = {
    {ParameterId::OilPressure, 2U, RawType::Unsigned16, ByteOrder::Little,
     0.0003333333f, 0.0f, 0.0f, 10.0f, kTimeout},
    {ParameterId::OilTemperature, 4U, RawType::Unsigned8, ByteOrder::Little,
     1.0f, -40.0f, -40.0f, 215.0f, kTimeout},
    {ParameterId::BatteryVoltage, 6U, RawType::Unsigned16, ByteOrder::Little,
     0.001f, 0.0f, 0.0f, 25.0f, kTimeout},
    {ParameterId::FuelTemperature, 5U, RawType::Unsigned8, ByteOrder::Little,
     1.0f, -40.0f, -40.0f, 215.0f, kTimeout},
};
const CanSignalDefinition kAirFuel[] = {
    {ParameterId::MassAirFlow, 2U, RawType::Unsigned16, ByteOrder::Little,
     0.0027777778f, 0.0f, 0.0f, 2000.0f, kTimeout},
    {ParameterId::InjectorPulseWidth, 4U, RawType::Unsigned16,
     ByteOrder::Little, 0.0033333333f, 0.0f, 0.0f, 100.0f, kTimeout},
};
const CanSignalDefinition kFueling[] = {
    {ParameterId::Lambda, 0U, RawType::Unsigned16, ByteOrder::Little, 0.0001f,
     0.0f, 0.0f, 2.0f, kTimeout},
    {ParameterId::FuelPressure, 4U, RawType::Unsigned16, ByteOrder::Little,
     0.0003333333f, 0.0f, 0.0f, 10.0f, kTimeout},
    {ParameterId::Lambda2, 2U, RawType::Unsigned16, ByteOrder::Little, 0.0001f,
     0.0f, 0.0f, 2.0f, kTimeout},
};
const CanSignalDefinition kEgt[] = {
    {ParameterId::Egt1, 0U, RawType::Unsigned8, ByteOrder::Little, 5.0f, 0.0f, 0.0f, 1275.0f, kTimeout},
    {ParameterId::Egt2, 1U, RawType::Unsigned8, ByteOrder::Little, 5.0f, 0.0f, 0.0f, 1275.0f, kTimeout},
    {ParameterId::Egt3, 2U, RawType::Unsigned8, ByteOrder::Little, 5.0f, 0.0f, 0.0f, 1275.0f, kTimeout},
    {ParameterId::Egt4, 3U, RawType::Unsigned8, ByteOrder::Little, 5.0f, 0.0f, 0.0f, 1275.0f, kTimeout},
    {ParameterId::Egt5, 4U, RawType::Unsigned8, ByteOrder::Little, 5.0f, 0.0f, 0.0f, 1275.0f, kTimeout},
    {ParameterId::Egt6, 5U, RawType::Unsigned8, ByteOrder::Little, 5.0f, 0.0f, 0.0f, 1275.0f, kTimeout},
    {ParameterId::Egt7, 6U, RawType::Unsigned8, ByteOrder::Little, 5.0f, 0.0f, 0.0f, 1275.0f, kTimeout},
    {ParameterId::Egt8, 7U, RawType::Unsigned8, ByteOrder::Little, 5.0f, 0.0f, 0.0f, 1275.0f, kTimeout},
};
const CanFrameDefinition kFrames[] = {
    {0x200U, false, 8U, 0U, 0U, 0U, kStatus,
     sizeof(kStatus) / sizeof(kStatus[0])},
    {0x201U, false, 8U, 0U, 0U, 0U, kSpeeds, 5U},
    {0x202U, false, 8U, 0U, 0U, 0U, kThrottle, 2U},
    {0x203U, false, 8U, 0U, 0U, 0U, kSensors1, 3U},
    {0x204U, false, 8U, 0U, 0U, 0U, kSensors2, 4U},
    {0x205U, false, 8U, 0U, 0U, 0U, kAirFuel, 2U},
    {0x207U, false, 8U, 0U, 0U, 0U, kFueling, 3U},
    {0x209U, false, 8U, 0U, 0U, 0U, kEgt, 8U},
    {0x20BU, false, 8U, 0U, 0U, 0U, kStatus11,
     sizeof(kStatus11) / sizeof(kStatus11[0])},
};
}  // namespace

const CanProfile kRusefiVerboseProfile{
    "rusefi_verbose", "rusEFI verbose", "rusEFI", "verbose CAN DBC",
    CanProfileVerification::Verified, 500000U,
    "https://github.com/rusefi/rusefi/blob/71a3239be9dbb9a06b12ba15dd8b2bdd5b8cee32/firmware/controllers/can/rusEFI_CAN_verbose.dbc",
    "71a3239be9dbb9a06b12ba15dd8b2bdd5b8cee32",
    "Default standard-ID base 0x200", kFrames,
    sizeof(kFrames) / sizeof(kFrames[0])};
