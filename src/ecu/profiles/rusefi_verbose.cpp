#include "ecu/profiles/profile_declarations.h"

namespace {
constexpr uint32_t kTimeout = 300U;
const CanSignalDefinition kStatus[] = {
    {ParameterId::Gear, 5U, RawType::Unsigned8, ByteOrder::Little, 1.0f, 0.0f,
     0.0f, 20.0f, kTimeout},
};
const CanSignalDefinition kSpeeds[] = {
    {ParameterId::Rpm, 0U, RawType::Unsigned16, ByteOrder::Little, 1.0f, 0.0f,
     0.0f, 30000.0f, kTimeout},
    {ParameterId::Speed, 6U, RawType::Unsigned8, ByteOrder::Little, 1.0f, 0.0f,
     0.0f, 255.0f, kTimeout},
};
const CanSignalDefinition kThrottle[] = {
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
};
const CanSignalDefinition kFueling[] = {
    {ParameterId::Lambda, 0U, RawType::Unsigned16, ByteOrder::Little, 0.0001f,
     0.0f, 0.0f, 2.0f, kTimeout},
    {ParameterId::FuelPressure, 4U, RawType::Unsigned16, ByteOrder::Little,
     0.0003333333f, 0.0f, 0.0f, 10.0f, kTimeout},
};
const CanFrameDefinition kFrames[] = {
    {0x200U, false, 8U, 0U, 0U, 0U, kStatus, 1U},
    {0x201U, false, 8U, 0U, 0U, 0U, kSpeeds, 2U},
    {0x202U, false, 8U, 0U, 0U, 0U, kThrottle, 1U},
    {0x203U, false, 8U, 0U, 0U, 0U, kSensors1, 3U},
    {0x204U, false, 8U, 0U, 0U, 0U, kSensors2, 3U},
    {0x207U, false, 8U, 0U, 0U, 0U, kFueling, 2U},
};
}  // namespace

const CanProfile kRusefiVerboseProfile{
    "rusefi_verbose", "rusEFI verbose", "rusEFI", "verbose CAN DBC",
    CanProfileVerification::Verified, 500000U,
    "https://github.com/rusefi/rusefi/blob/71a3239be9dbb9a06b12ba15dd8b2bdd5b8cee32/firmware/controllers/can/rusEFI_CAN_verbose.dbc",
    "71a3239be9dbb9a06b12ba15dd8b2bdd5b8cee32",
    "Default standard-ID base 0x200", kFrames,
    sizeof(kFrames) / sizeof(kFrames[0])};
