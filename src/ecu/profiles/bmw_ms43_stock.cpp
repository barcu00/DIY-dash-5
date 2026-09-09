#include "ecu/profiles/profile_declarations.h"

namespace {
constexpr uint32_t kFastTimeout = 100U;

constexpr CanSignalDefinition flag(ParameterId parameter, uint8_t byte_offset,
                                   uint32_t mask, uint8_t shift) {
    return {parameter, byte_offset, RawType::Unsigned8, ByteOrder::Little,
            1.0f, 0.0f, 0.0f, 1.0f, kFastTimeout,
            CanSignalKind::MaskedFlag, mask, shift, 1ULL << 1U};
}

const CanSignalDefinition kFrame316[] = {
    {ParameterId::Rpm, 2U, RawType::Unsigned16, ByteOrder::Little,
     0.15625f, 0.0f, 0.0f, 10240.0f, kFastTimeout},
    flag(ParameterId::IgnitionOn, 0U, 0x01U, 0U),
    flag(ParameterId::CrankSensorError, 0U, 0x02U, 1U),
    flag(ParameterId::MassAirFlowError, 0U, 0x80U, 7U),
};

const CanSignalDefinition kFrame329[] = {
    {ParameterId::Clt, 1U, RawType::Unsigned8, ByteOrder::Little,
     0.75f, -48.0f, -48.0f, 150.0f, kFastTimeout},
    {ParameterId::BarometricPressure, 2U, RawType::Unsigned8,
     ByteOrder::Little, 0.002f, 0.598f, 0.5f, 1.2f, kFastTimeout},
    {ParameterId::AcceleratorPosition, 5U, RawType::Unsigned8,
     ByteOrder::Little, 0.390625f, 0.0f, 0.0f, 100.0f, kFastTimeout},
    flag(ParameterId::ClutchPressed, 3U, 0x01U, 0U),
    flag(ParameterId::IdleActive, 3U, 0x02U, 1U),
    flag(ParameterId::EngineRunning, 3U, 0x08U, 3U),
    flag(ParameterId::BrakePressed, 6U, 0x01U, 0U),
    flag(ParameterId::BrakeSystemFault, 6U, 0x02U, 1U),
    flag(ParameterId::KickdownActive, 6U, 0x04U, 2U),
};

const CanSignalDefinition kFrame545[] = {
    {ParameterId::OilTemperature, 4U, RawType::Unsigned8, ByteOrder::Little,
     1.0f, -48.0f, -48.0f, 207.0f, kFastTimeout},
    flag(ParameterId::CheckEngine, 0U, 0x02U, 1U),
    flag(ParameterId::EmlWarning, 0U, 0x10U, 4U),
    flag(ParameterId::OilConsumptionWarning, 3U, 0x01U, 0U),
    flag(ParameterId::OilLossWarning, 3U, 0x02U, 1U),
    flag(ParameterId::OilSensorFault, 3U, 0x04U, 2U),
    flag(ParameterId::CoolantOverheat, 3U, 0x08U, 3U),
    flag(ParameterId::UpshiftRequest, 3U, 0x80U, 7U),
    flag(ParameterId::LowOilPressure, 7U, 0x80U, 7U),
};

const CanFrameDefinition kFrames[] = {
    {0x316U, false, 8U, 0U, 0U, 0U, kFrame316,
     sizeof(kFrame316) / sizeof(kFrame316[0])},
    {0x329U, false, 8U, 0U, 0U, 0U, kFrame329,
     sizeof(kFrame329) / sizeof(kFrame329[0])},
    {0x545U, false, 8U, 0U, 0U, 0U, kFrame545,
     sizeof(kFrame545) / sizeof(kFrame545[0])},
};
}  // namespace

const CanProfile kBmwMs43StockProfile{
    "bmw_ms43_stock", "BMW MS43 Stock", "BMW Siemens MS43",
    "Stock E46 powertrain CAN", CanProfileVerification::Verified, 500000U,
    "https://www.ms4x.net/index.php?title=Siemens_MS43_CAN_Bus&oldid=23051",
    "MS4X oldid 23051",
    "Stock 11-bit 500 kbit/s receive-only frames; no OLM or custom 0x33C",
    kFrames, sizeof(kFrames) / sizeof(kFrames[0])};
