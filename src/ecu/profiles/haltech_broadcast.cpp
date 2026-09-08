#include "ecu/profiles/profile_declarations.h"

namespace {
constexpr uint32_t kFast = 100U;
constexpr uint32_t kMedium = 200U;
constexpr uint32_t kSlow = 300U;
const CanSignalDefinition kData1[] = {
    {ParameterId::Rpm, 0U, RawType::Unsigned16, ByteOrder::Big, 1.0f, 0.0f,
     0.0f, 30000.0f, kFast},
    {ParameterId::Map, 2U, RawType::Unsigned16, ByteOrder::Big, 0.001f, 0.0f,
     0.0f, 30.0f, kFast},
    {ParameterId::Tps, 4U, RawType::Unsigned16, ByteOrder::Big, 0.1f, 0.0f,
     0.0f, 100.0f, kFast},
};
const CanSignalDefinition kData2[] = {
    {ParameterId::FuelPressure, 0U, RawType::Unsigned16, ByteOrder::Big,
     0.001f, -1.013f, -1.013f, 30.0f, kFast},
    {ParameterId::OilPressure, 2U, RawType::Unsigned16, ByteOrder::Big,
     0.001f, -1.013f, -1.013f, 30.0f, kFast},
};
const CanSignalDefinition kLambda[] = {
    {ParameterId::Lambda, 0U, RawType::Unsigned16, ByteOrder::Big, 0.001f,
     0.0f, 0.0f, 3.0f, kMedium},
};
const CanSignalDefinition kVss[] = {
    {ParameterId::Speed, 0U, RawType::Unsigned16, ByteOrder::Big, 0.1f, 0.0f,
     0.0f, 500.0f, kMedium},
    {ParameterId::Gear, 3U, RawType::Unsigned8, ByteOrder::Big, 1.0f, 0.0f,
     0.0f, 20.0f, kMedium},
};
const CanSignalDefinition kData4[] = {
    {ParameterId::BatteryVoltage, 0U, RawType::Unsigned16, ByteOrder::Big,
     0.1f, 0.0f, 0.0f, 30.0f, kSlow},
};
const CanSignalDefinition kData5[] = {
    {ParameterId::Clt, 0U, RawType::Unsigned16, ByteOrder::Big, 0.1f, -273.15f,
     -100.0f, 300.0f, kSlow},
    {ParameterId::Iat, 2U, RawType::Unsigned16, ByteOrder::Big, 0.1f, -273.15f,
     -100.0f, 300.0f, kSlow},
    {ParameterId::OilTemperature, 6U, RawType::Unsigned16, ByteOrder::Big,
     0.1f, -273.15f, -100.0f, 300.0f, kSlow},
};
const CanFrameDefinition kFrames[] = {
    {0x360U, false, 8U, 0U, 0U, 0U, kData1, 3U},
    {0x361U, false, 8U, 0U, 0U, 0U, kData2, 2U},
    {0x368U, false, 8U, 0U, 0U, 0U, kLambda, 1U},
    {0x370U, false, 8U, 0U, 0U, 0U, kVss, 2U},
    {0x372U, false, 8U, 0U, 0U, 0U, kData4, 1U},
    {0x3E0U, false, 8U, 0U, 0U, 0U, kData5, 3U},
};
}  // namespace

const CanProfile kHaltechBroadcastProfile{
    "haltech_broadcast_2_0", "Haltech Broadcast 2.0", "Haltech Elite/Nexus",
    "2.0", CanProfileVerification::Verified, 1000000U,
    "https://support.haltech.com/portal/en/kb/articles/haltech-can-ecu-broadcast-protocol",
    "document 2.0", "Big-endian Haltech ECU broadcast", kFrames,
    sizeof(kFrames) / sizeof(kFrames[0])};
