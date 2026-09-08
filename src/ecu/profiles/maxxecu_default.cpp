#include "ecu/profiles/profile_declarations.h"

namespace {
constexpr uint32_t kFast = 100U;
constexpr uint32_t kSlow = 300U;
const CanSignalDefinition kFast1[] = {
    {ParameterId::Rpm, 0U, RawType::Signed16, ByteOrder::Little, 1.0f, 0.0f,
     0.0f, 30000.0f, kFast},
    {ParameterId::Tps, 2U, RawType::Signed16, ByteOrder::Little, 0.1f, 0.0f,
     0.0f, 100.0f, kFast},
    {ParameterId::Map, 4U, RawType::Signed16, ByteOrder::Little, 0.001f, 0.0f,
     0.0f, 30.0f, kFast},
    {ParameterId::Lambda, 6U, RawType::Signed16, ByteOrder::Little, 0.001f,
     0.0f, 0.0f, 3.0f, kFast},
};
const CanSignalDefinition kFast3[] = {
    {ParameterId::Speed, 6U, RawType::Signed16, ByteOrder::Little, 0.1f, 0.0f,
     0.0f, 500.0f, kFast},
};
const CanSignalDefinition kSlow1[] = {
    {ParameterId::BatteryVoltage, 0U, RawType::Signed16, ByteOrder::Little,
     0.01f, 0.0f, 0.0f, 30.0f, kSlow},
    {ParameterId::Iat, 4U, RawType::Signed16, ByteOrder::Little, 0.1f, 0.0f,
     -100.0f, 300.0f, kSlow},
    {ParameterId::Clt, 6U, RawType::Signed16, ByteOrder::Little, 0.1f, 0.0f,
     -100.0f, 300.0f, kSlow},
};
const CanSignalDefinition kSlow7[] = {
    {ParameterId::Gear, 0U, RawType::Signed16, ByteOrder::Little, 1.0f, 0.0f,
     -1.0f, 20.0f, kSlow},
    {ParameterId::OilPressure, 4U, RawType::Signed16, ByteOrder::Little,
     0.001f, 0.0f, 0.0f, 30.0f, kSlow},
    {ParameterId::OilTemperature, 6U, RawType::Signed16, ByteOrder::Little,
     0.1f, 0.0f, -100.0f, 300.0f, kSlow},
};
const CanSignalDefinition kSlow8[] = {
    {ParameterId::FuelPressure, 0U, RawType::Signed16, ByteOrder::Little,
     0.001f, 0.0f, 0.0f, 30.0f, kSlow},
};
const CanFrameDefinition kFrames[] = {
    {0x520U, false, 8U, 0U, 0U, 0U, kFast1, 4U},
    {0x522U, false, 8U, 0U, 0U, 0U, kFast3, 1U},
    {0x530U, false, 8U, 0U, 0U, 0U, kSlow1, 3U},
    {0x536U, false, 8U, 0U, 0U, 0U, kSlow7, 3U},
    {0x537U, false, 8U, 0U, 0U, 0U, kSlow8, 1U},
};
}  // namespace

const CanProfile kMaxxEcuDefaultProfile{
    "maxxecu_default_1_3", "MaxxECU Default 1.3", "MaxxECU", "1.3",
    CanProfileVerification::Verified, 500000U,
    "https://www.maxxecu.com/webhelp/can-default_maxxecu_protocol.html",
    "protocol 1.3; 2020-09-29", "11-bit little-endian default output",
    kFrames, sizeof(kFrames) / sizeof(kFrames[0])};
