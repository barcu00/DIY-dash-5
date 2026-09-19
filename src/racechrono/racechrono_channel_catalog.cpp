#include "racechrono_channel_catalog.h"

#include <array>

namespace {
constexpr uint32_t kStaleMs = 5000U;
constexpr RaceChronoValueEncoding kScaled =
    RaceChronoValueEncoding::ScaledSignedInt32;
constexpr RaceChronoValueEncoding kCoordinate =
    RaceChronoValueEncoding::CoordinateDegreesTimes6000000;

constexpr std::array<RaceChronoChannelDescriptor, 33U> kChannels{{
    {ParameterId::RcLapNumber, 1U, "channel(device(lap), lap_number)", 1.0f, kScaled, kStaleMs},
    {ParameterId::RcLapTime, 2U, "channel(device(lap), lap_time) * 1000", 0.001f, kScaled, kStaleMs},
    {ParameterId::RcLapDistance, 3U, "channel(device(lap), lap_distance) * 10", 0.1f, kScaled, kStaleMs},
    {ParameterId::RcPreviousLapNumber, 4U, "channel(device(lap), previous_lap_number)", 1.0f, kScaled, kStaleMs},
    {ParameterId::RcPreviousLapTime, 5U, "channel(device(lap), previous_lap_time) * 1000", 0.001f, kScaled, kStaleMs},
    {ParameterId::RcBestLapNumber, 6U, "channel(device(lap), best_lap_number)", 1.0f, kScaled, kStaleMs},
    {ParameterId::RcBestLapTime, 7U, "channel(device(lap), best_lap_time) * 1000", 0.001f, kScaled, kStaleMs},
    {ParameterId::RcComparisonLapNumber, 8U, "channel(device(lap), comparison_lap_number)", 1.0f, kScaled, kStaleMs},
    {ParameterId::RcComparisonLapTime, 9U, "channel(device(lap), comparison_lap_time) * 1000", 0.001f, kScaled, kStaleMs},
    {ParameterId::RcDeltaLapTime, 10U, "channel(device(lap), delta_lap_time) * 1000", 0.001f, kScaled, kStaleMs},
    {ParameterId::RcLapTimeGain, 11U, "channel(device(lap), lap_time_gain) * 1000", 0.001f, kScaled, kStaleMs},
    {ParameterId::RcSectorTime, 12U, "channel(device(lap), sector_time) * 1000", 0.001f, kScaled, kStaleMs},
    {ParameterId::RcSectorDistance, 13U, "channel(device(lap), sector_distance) * 10", 0.1f, kScaled, kStaleMs},
    {ParameterId::RcTotalRaceTime, 14U, "channel(device(lap), total_race_time) * 1000", 0.001f, kScaled, kStaleMs},
    {ParameterId::RcTotalRaceDistance, 15U, "channel(device(lap), total_race_distance) * 10", 0.1f, kScaled, kStaleMs},
    {ParameterId::RcElapsedTime, 16U, "channel(device(gps), elapsed_time) * 1000", 0.001f, kScaled, kStaleMs},
    {ParameterId::RcGpsSpeed, 17U, "channel(device(gps), speed) * 360", 0.01f, kScaled, kStaleMs},
    {ParameterId::RcAltitude, 18U, "channel(device(gps), altitude) * 10", 0.1f, kScaled, kStaleMs},
    {ParameterId::RcBearing, 19U, "channel(device(gps), bearing) * 100", 0.01f, kScaled, kStaleMs},
    {ParameterId::RcLatitude, 20U, "channel(device(gps), latitude)", 1.0f / 6000000.0f, kCoordinate, kStaleMs},
    {ParameterId::RcLongitude, 21U, "channel(device(gps), longitude)", 1.0f / 6000000.0f, kCoordinate, kStaleMs},
    {ParameterId::RcSatellites, 22U, "channel(device(gps), satellites)", 1.0f, kScaled, kStaleMs},
    {ParameterId::RcFixType, 23U, "channel(device(gps), fix_type)", 1.0f, kScaled, kStaleMs},
    {ParameterId::RcAccuracy, 24U, "channel(device(gps), accuracy) * 100", 0.01f, kScaled, kStaleMs},
    {ParameterId::RcCoordinatePrecision, 25U, "channel(device(gps), coordinate_precision) * 100", 0.01f, kScaled, kStaleMs},
    {ParameterId::RcAltitudePrecision, 26U, "channel(device(gps), altitude_precision) * 100", 0.01f, kScaled, kStaleMs},
    {ParameterId::RcThreeDPrecision, 27U, "channel(device(gps), 3d_precision) * 100", 0.01f, kScaled, kStaleMs},
    {ParameterId::RcDeviceUpdateRate, 28U, "channel(device(gps), device_update_rate) * 10", 0.1f, kScaled, kStaleMs},
    {ParameterId::RcDeltaSpeed, 29U, "channel(device(calc), delta_speed) * 360", 0.01f, kScaled, kStaleMs},
    {ParameterId::RcLateralAcceleration, 30U, "channel(device(calc), lateral_acc) * 1000", 0.001f, kScaled, kStaleMs},
    {ParameterId::RcLongitudinalAcceleration, 31U, "channel(device(calc), longitudinal_acc) * 1000", 0.001f, kScaled, kStaleMs},
    {ParameterId::RcCombinedAcceleration, 32U, "channel(device(calc), combined_acc) * 1000", 0.001f, kScaled, kStaleMs},
    {ParameterId::RcLeanAngle, 33U, "channel(device(calc), lean_angle) * 100", 0.01f, kScaled, kStaleMs},
}};
}  // namespace

bool isRaceChronoParameter(ParameterId id) {
    const auto value = static_cast<uint8_t>(id);
    return value >= static_cast<uint8_t>(ParameterId::RcLapNumber) &&
           value <= static_cast<uint8_t>(ParameterId::RcLeanAngle);
}

const RaceChronoChannelDescriptor* raceChronoChannel(ParameterId id) {
    if (!isRaceChronoParameter(id)) {
        return nullptr;
    }
    const std::size_t index = static_cast<uint8_t>(id) -
                              static_cast<uint8_t>(ParameterId::RcLapNumber);
    return &kChannels[index];
}

const RaceChronoChannelDescriptor* raceChronoChannelByMonitorId(
    uint8_t monitor_id) {
    if (monitor_id == 0U || monitor_id > kChannels.size()) {
        return nullptr;
    }
    return &kChannels[monitor_id - 1U];
}

std::size_t raceChronoChannelCount() {
    return kChannels.size();
}

const RaceChronoChannelDescriptor& raceChronoChannelAt(std::size_t index) {
    return kChannels[index];
}
