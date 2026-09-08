#include "ecu/profiles/profile_declarations.h"

namespace {
constexpr uint32_t kTimeout = 500U;
const CanSignalDefinition kIndex0[] = {
    {ParameterId::Rpm, 2U, RawType::Unsigned16, ByteOrder::Little, 1.0f, 0.0f,
     0.0f, 30000.0f, kTimeout},
    {ParameterId::Map, 4U, RawType::Unsigned16, ByteOrder::Little, 0.01f, 0.0f,
     0.0f, 30.0f, kTimeout},
};
const CanSignalDefinition kIndex1[] = {
    {ParameterId::BarometricPressure, 2U, RawType::Unsigned16,
     ByteOrder::Little, 0.001f, 0.0f, 0.0f, 3.0f, kTimeout},
    {ParameterId::Tps, 4U, RawType::Unsigned16, ByteOrder::Little, 0.1f, 0.0f,
     0.0f, 100.0f, kTimeout},
    {ParameterId::InjectorDuty, 6U, RawType::Unsigned16, ByteOrder::Little,
     0.1f, 0.0f, 0.0f, 200.0f, kTimeout},
};
const CanSignalDefinition kIndex2[] = {
    {ParameterId::InjectorPulseWidth, 4U, RawType::Unsigned16,
     ByteOrder::Little, 0.001f, 0.0f, 0.0f, 100.0f, kTimeout},
    {ParameterId::Clt, 6U, RawType::Unsigned16, ByteOrder::Little, 1.0f, -50.0f,
     -50.0f, 300.0f, kTimeout},
};
const CanSignalDefinition kIndex3[] = {
    {ParameterId::Iat, 2U, RawType::Unsigned16, ByteOrder::Little, 1.0f, -50.0f,
     -50.0f, 300.0f, kTimeout},
    {ParameterId::BatteryVoltage, 4U, RawType::Unsigned16, ByteOrder::Little,
     0.01f, 0.0f, 0.0f, 30.0f, kTimeout},
    {ParameterId::MassAirFlow, 6U, RawType::Unsigned16, ByteOrder::Little,
     0.1f, 0.0f, 0.0f, 2000.0f, kTimeout},
};
const CanSignalDefinition kIndex4[] = {
    {ParameterId::Gear, 2U, RawType::Unsigned16, ByteOrder::Little, 1.0f, 0.0f,
     0.0f, 20.0f, kTimeout},
    {ParameterId::IgnitionTiming, 6U, RawType::Unsigned16, ByteOrder::Little,
     0.1f, -100.0f, -100.0f, 100.0f, kTimeout},
};
const CanSignalDefinition kIndex6[] = {
    {ParameterId::Lambda, 4U, RawType::Unsigned16, ByteOrder::Little, 0.001f,
     0.0f, 0.0f, 3.0f, kTimeout},
    {ParameterId::Lambda2, 6U, RawType::Unsigned16, ByteOrder::Little, 0.001f,
     0.0f, 0.0f, 3.0f, kTimeout},
};
const CanSignalDefinition kIndex7[] = {
    {ParameterId::FuelPressure, 6U, RawType::Unsigned16, ByteOrder::Little,
     0.01f, 0.0f, 0.0f, 30.0f, kTimeout},
};
const CanSignalDefinition kIndex8[] = {
    {ParameterId::OilTemperature, 2U, RawType::Unsigned16, ByteOrder::Little,
     1.0f, -50.0f, -50.0f, 300.0f, kTimeout},
    {ParameterId::OilPressure, 4U, RawType::Unsigned16, ByteOrder::Little,
     0.01f, 0.0f, 0.0f, 30.0f, kTimeout},
    {ParameterId::WheelSpeedLf, 6U, RawType::Unsigned16, ByteOrder::Little,
     0.1f, 0.0f, 0.0f, 500.0f, kTimeout},
};
const CanSignalDefinition kIndex9[] = {
    {ParameterId::WheelSpeedLr, 2U, RawType::Unsigned16, ByteOrder::Little,
     0.1f, 0.0f, 0.0f, 500.0f, kTimeout},
    {ParameterId::WheelSpeedRf, 4U, RawType::Unsigned16, ByteOrder::Little,
     0.1f, 0.0f, 0.0f, 500.0f, kTimeout},
    {ParameterId::WheelSpeedRr, 6U, RawType::Unsigned16, ByteOrder::Little,
     0.1f, 0.0f, 0.0f, 500.0f, kTimeout},
};
const CanSignalDefinition kIndex13[] = {
    {ParameterId::AcceleratorPosition, 2U, RawType::Unsigned16,
     ByteOrder::Little, 0.1f, 0.0f, 0.0f, 100.0f, kTimeout},
    {ParameterId::EthanolContent, 4U, RawType::Unsigned16, ByteOrder::Little,
     0.1f, 0.0f, 0.0f, 100.0f, kTimeout},
};
const CanFrameDefinition kFrames[] = {
    {0x3E8U, false, 8U, 0U, 0xFFFFU, 0U, kIndex0, 2U},
    {0x3E8U, false, 8U, 0U, 0xFFFFU, 1U, kIndex1, 3U},
    {0x3E8U, false, 8U, 0U, 0xFFFFU, 2U, kIndex2, 2U},
    {0x3E8U, false, 8U, 0U, 0xFFFFU, 3U, kIndex3, 3U},
    {0x3E8U, false, 8U, 0U, 0xFFFFU, 4U, kIndex4, 2U},
    {0x3E8U, false, 8U, 0U, 0xFFFFU, 6U, kIndex6, 2U},
    {0x3E8U, false, 8U, 0U, 0xFFFFU, 7U, kIndex7, 1U},
    {0x3E8U, false, 8U, 0U, 0xFFFFU, 8U, kIndex8, 3U},
    {0x3E8U, false, 8U, 0U, 0xFFFFU, 9U, kIndex9, 3U},
    {0x3E8U, false, 8U, 0U, 0xFFFFU, 13U, kIndex13, 2U},
};
}  // namespace

const CanProfile kLinkGenericDashProfile{
    "link_generic_dash_experimental", "Link Generic Dash (experimental)",
    "Link G4+/G4X/G5", "Generic Dash",
    CanProfileVerification::Experimental, 1000000U,
    "https://kb.linkecu.com/acc-kb/latest/can-gauge-available-channels",
    "LinkGenericDash cd7426e872a40a812880e0b1c6af9c39391616c1",
    "Official channels; community byte layout; ECU stream ID must be 1000",
    kFrames, sizeof(kFrames) / sizeof(kFrames[0])};
