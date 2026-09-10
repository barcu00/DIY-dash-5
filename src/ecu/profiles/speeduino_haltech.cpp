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
const CanSignalDefinition kData3[] = {
    {ParameterId::InjectorDuty, 0U, RawType::Unsigned16, ByteOrder::Big,
     1.0f, 0.0f, 0.0f, 100.0f, kFast},
    {ParameterId::IgnitionTiming, 4U, RawType::Signed16, ByteOrder::Big,
     0.1f, 0.0f, -100.0f, 100.0f, kFast},
};
const CanSignalDefinition kData7[] = {
    {ParameterId::InjectorPulseWidth, 0U, RawType::Unsigned16, ByteOrder::Big,
     0.001f, 0.0f, 0.0f, 100.0f, kFast},
};
const CanSignalDefinition kLambda[] = {
    {ParameterId::Lambda, 0U, RawType::Unsigned16, ByteOrder::Big, 0.001f,
     0.0f, 0.0f, 3.0f, kMedium},
    {ParameterId::Lambda2, 2U, RawType::Unsigned16, ByteOrder::Big, 0.001f,
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
    {ParameterId::BoostTarget, 4U, RawType::Unsigned16, ByteOrder::Big,
     0.001f, 0.0f, 0.0f, 30.0f, kSlow},
    {ParameterId::BarometricPressure, 6U, RawType::Unsigned16, ByteOrder::Big,
     0.001f, 0.0f, 0.0f, 3.0f, kSlow},
};
const CanSignalDefinition kData5[] = {
    {ParameterId::Clt, 0U, RawType::Unsigned16, ByteOrder::Big, 0.1f, -273.0f,
     -100.0f, 300.0f, kSlow},
    {ParameterId::Iat, 2U, RawType::Unsigned16, ByteOrder::Big, 0.1f, -273.0f,
     -100.0f, 300.0f, kSlow},
    {ParameterId::FuelTemperature, 4U, RawType::Unsigned16, ByteOrder::Big,
     0.1f, -273.0f, -100.0f, 300.0f, kSlow},
};
const CanFrameDefinition kFrames[] = {
    {0x360U, false, 8U, 0U, 0U, 0U, kData1, 3U},
    {0x361U, false, 8U, 0U, 0U, 0U, kData2, 2U},
    {0x362U, false, 8U, 0U, 0U, 0U, kData3, 2U},
    {0x364U, false, 8U, 0U, 0U, 0U, kData7, 1U},
    {0x368U, false, 8U, 0U, 0U, 0U, kLambda, 2U},
    {0x370U, false, 8U, 0U, 0U, 0U, kVss, 2U},
    {0x372U, false, 8U, 0U, 0U, 0U, kData4, 3U},
    {0x3E0U, false, 8U, 0U, 0U, 0U, kData5, 3U},
};
}  // namespace

const CanProfile kSpeeduinoHaltechProfile{
    "speeduino_haltech", "Speeduino Haltech mode", "Speeduino",
    "Haltech-compatible broadcast",
    CanProfileVerification::Verified, 500000U,
    "https://github.com/speeduino/speeduino/blob/5275fbaf82e57b364d9e0d0b4cbadbc33f95c668/speeduino/comms_CAN.cpp",
    "5275fbaf82e57b364d9e0d0b4cbadbc33f95c668",
    "Only fields populated by the pinned transmitter are exposed", kFrames,
    sizeof(kFrames) / sizeof(kFrames[0])};
