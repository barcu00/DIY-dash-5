#pragma once

#include <cstddef>
#include <cstdint>

#include "telemetry/parameter_id.h"

enum class RaceChronoValueEncoding : uint8_t {
    ScaledSignedInt32,
    CoordinateDegreesTimes6000000,
};

struct RaceChronoChannelDescriptor {
    ParameterId parameter;
    uint8_t monitor_id;
    const char* equation;
    float raw_to_native;
    RaceChronoValueEncoding encoding;
    uint32_t stale_ms;
};

bool isRaceChronoParameter(ParameterId id);
const RaceChronoChannelDescriptor* raceChronoChannel(ParameterId id);
const RaceChronoChannelDescriptor* raceChronoChannelByMonitorId(uint8_t monitor_id);
std::size_t raceChronoChannelCount();
const RaceChronoChannelDescriptor& raceChronoChannelAt(std::size_t index);
