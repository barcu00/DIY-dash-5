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
    {ParameterId::InjectorPulseWidth, 0U, RawType::Signed16,
     ByteOrder::Little, 0.01f, 0.0f, 0.0f, 100.0f, kFast},
    {ParameterId::InjectorDuty, 2U, RawType::Signed16, ByteOrder::Little,
     0.1f, 0.0f, 0.0f, 200.0f, kFast},
    {ParameterId::Speed, 6U, RawType::Signed16, ByteOrder::Little, 0.1f, 0.0f,
     0.0f, 500.0f, kFast},
};
const CanSignalDefinition kFast2[] = {
    {ParameterId::Lambda2, 2U, RawType::Signed16, ByteOrder::Little, 0.001f,
     0.0f, 0.0f, 3.0f, kFast},
    {ParameterId::IgnitionTiming, 4U, RawType::Signed16, ByteOrder::Little,
     0.1f, 0.0f, -100.0f, 100.0f, kFast},
};
const CanSignalDefinition kSlow1[] = {
    {ParameterId::BatteryVoltage, 0U, RawType::Signed16, ByteOrder::Little,
     0.01f, 0.0f, 0.0f, 30.0f, kSlow},
    {ParameterId::BarometricPressure, 2U, RawType::Signed16,
     ByteOrder::Little, 0.001f, 0.0f, 0.0f, 3.0f, kSlow},
    {ParameterId::Iat, 4U, RawType::Signed16, ByteOrder::Little, 0.1f, 0.0f,
     -100.0f, 300.0f, kSlow},
    {ParameterId::Clt, 6U, RawType::Signed16, ByteOrder::Little, 0.1f, 0.0f,
     -100.0f, 300.0f, kSlow},
};
const CanSignalDefinition kSlow2[] = {
    {ParameterId::EthanolContent, 2U, RawType::Signed16, ByteOrder::Little,
     0.1f, 0.0f, 0.0f, 100.0f, kSlow},
    {ParameterId::Egt1, 6U, RawType::Signed16, ByteOrder::Little, 1.0f, 0.0f,
     -100.0f, 1400.0f, kSlow},
};
const CanSignalDefinition kSlow3[] = {
    {ParameterId::Egt2, 0U, RawType::Signed16, ByteOrder::Little, 1.0f, 0.0f, -100.0f, 1400.0f, kSlow},
    {ParameterId::Egt3, 2U, RawType::Signed16, ByteOrder::Little, 1.0f, 0.0f, -100.0f, 1400.0f, kSlow},
    {ParameterId::Egt4, 4U, RawType::Signed16, ByteOrder::Little, 1.0f, 0.0f, -100.0f, 1400.0f, kSlow},
    {ParameterId::Egt5, 6U, RawType::Signed16, ByteOrder::Little, 1.0f, 0.0f, -100.0f, 1400.0f, kSlow},
};
const CanSignalDefinition kSlow4[] = {
    {ParameterId::Egt6, 0U, RawType::Signed16, ByteOrder::Little, 1.0f, 0.0f, -100.0f, 1400.0f, kSlow},
    {ParameterId::Egt7, 2U, RawType::Signed16, ByteOrder::Little, 1.0f, 0.0f, -100.0f, 1400.0f, kSlow},
    {ParameterId::Egt8, 4U, RawType::Signed16, ByteOrder::Little, 1.0f, 0.0f, -100.0f, 1400.0f, kSlow},
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
    {ParameterId::CoolantPressure, 4U, RawType::Signed16, ByteOrder::Little,
     0.001f, 0.0f, 0.0f, 30.0f, kSlow},
    {ParameterId::BoostTarget, 6U, RawType::Signed16, ByteOrder::Little,
     0.001f, 0.0f, 0.0f, 30.0f, kSlow},
};
const CanFrameDefinition kFrames[] = {
    {0x520U, false, 8U, 0U, 0U, 0U, kFast1, 4U},
    {0x521U, false, 8U, 0U, 0U, 0U, kFast2, 2U},
    {0x522U, false, 8U, 0U, 0U, 0U, kFast3, 3U},
    {0x530U, false, 8U, 0U, 0U, 0U, kSlow1, 4U},
    {0x531U, false, 8U, 0U, 0U, 0U, kSlow2, 2U},
    {0x532U, false, 8U, 0U, 0U, 0U, kSlow3, 4U},
    {0x533U, false, 8U, 0U, 0U, 0U, kSlow4, 3U},
    {0x536U, false, 8U, 0U, 0U, 0U, kSlow7, 3U},
    {0x537U, false, 8U, 0U, 0U, 0U, kSlow8, 3U},
};
}  // namespace

const CanProfile kMaxxEcuDefaultProfile{
    "maxxecu_default_1_3", "MaxxECU Default 1.3", "MaxxECU", "1.3",
    CanProfileVerification::Verified, 500000U,
    "https://www.maxxecu.com/webhelp/can-default_maxxecu_protocol.html",
    "protocol 1.3; 2020-09-29", "11-bit little-endian default output",
    kFrames, sizeof(kFrames) / sizeof(kFrames[0])};
