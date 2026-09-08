#include "ecu/profiles/profile_declarations.h"

namespace {
const CanSignalDefinition kFast[] = {
    {ParameterId::Rpm, 0U, RawType::Unsigned16, ByteOrder::Little, 0.125f, 0.0f,
     0.0f, 8196.0f, 100U},
    {ParameterId::Tps, 3U, RawType::Unsigned8, ByteOrder::Little, 0.5f, 0.0f,
     0.0f, 100.0f, 100U},
};
const CanSignalDefinition kSlow[] = {
    {ParameterId::Clt, 0U, RawType::Unsigned8, ByteOrder::Little, 1.0f, -40.0f,
     -40.0f, 214.0f, 500U},
    {ParameterId::OilTemperature, 5U, RawType::Unsigned8, ByteOrder::Little,
     1.0f, -40.0f, -40.0f, 214.0f, 500U},
    {ParameterId::Iat, 7U, RawType::Unsigned8, ByteOrder::Little, 1.0f, -40.0f,
     -40.0f, 214.0f, 500U},
};
const CanFrameDefinition kFrames[] = {
    {0x208U, false, 8U, 0U, 0U, 0U, kFast, 2U},
    {0x488U, false, 8U, 0U, 0U, 0U, kSlow, 3U},
};
}  // namespace

const CanProfile kPsaC2VtsProfile{
    "psa_c2_vts_engine_experimental",
    "PSA C2 VTS engine CAN (experimental)", "PSA AEE2004 HS.IS",
    "reverse-engineered", CanProfileVerification::Experimental, 500000U,
    "https://github.com/prototux/PSA-RE/tree/74294e99bd8f4decbfcdceabb11c7413dd977f4d/buses/AEE2004.full/HS.IS",
    "74294e99bd8f4decbfcdceabb11c7413dd977f4d",
    "Receive-only 0x208/0x488 subset; no VAN or diagnostics", kFrames,
    sizeof(kFrames) / sizeof(kFrames[0])};
