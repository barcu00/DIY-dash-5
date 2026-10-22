#pragma once

#include <array>
#include <cstdint>

#include "odometer/odometer_record.h"
#include "telemetry/vehicle_state.h"

struct OdometerSettings {
    DistanceSource source = DistanceSource::CanSpeed;
    bool gps_fallback = true;
    int16_t correction_permille = 0;  // -100..+100 = -10.0 %..+10.0 %
};

struct SpeedSample {
    float kph = 0.0f;
    bool valid = false;
};

// Integrates distance from a speed signal. Pure logic: persistence is driven
// by the caller through persistDue()/markPersisted()/persistFailed().
//
// - Trapezoidal integration of consecutive valid samples.
// - Gaps longer than kMaximumGapMs (lost data, stalls) are not bridged.
// - Speeds below kMinimumSpeedKph count as standstill (sensor noise).
// - Saved after kSaveWhileMovingM while driving, or as soon as the vehicle
//   has been stationary (or without speed data) for kStationarySaveMs.
//   Settings, SET and RESET are saved immediately.
class OdometerModel {
public:
    static constexpr uint32_t kMaximumGapMs = 250U;
    static constexpr float kMinimumSpeedKph = 1.0f;
    static constexpr double kSaveWhileMovingM = 1000.0;
    static constexpr uint32_t kStationarySaveMs = 2000U;
    static constexpr uint32_t kRetryAfterFailureMs = 30000U;
    static constexpr double kMaximumOdometerKm = 9999999.0;
    static constexpr int16_t kMaximumCorrectionPermille = 100;

    OdometerModel();

    void restore(const OdometerRecord& record, uint32_t now_ms);
    // Late load (storage was unavailable at boot): adds the stored totals to
    // the distance accumulated since boot instead of discarding either.
    void mergeStored(const OdometerRecord& record, uint32_t now_ms);
    OdometerRecord record() const;

    void integrate(SpeedSample sample, uint32_t now_ms);

    bool persistDue(uint32_t now_ms) const;
    void markPersisted(uint32_t now_ms);
    void persistFailed(uint32_t now_ms);
    bool storageError() const;

    bool setOdometerKm(double km, uint32_t now_ms);
    void resetTrip(TripId trip, uint32_t now_ms);
    bool setSettings(const OdometerSettings& settings);

    const OdometerSettings& settings() const;
    double odometerKm() const;
    double tripKm(TripId trip) const;
    uint64_t tripMovingMs(TripId trip) const;
    float tripAverageKph(TripId trip) const;
    double unsavedMeters() const;

    const SignalValue& get(ParameterId id) const;

private:
    void publish(uint32_t now_ms);
    void accumulate(double meters, uint32_t moving_ms);

    OdometerSettings settings_{};
    double odometer_m_ = 0.0;
    std::array<double, 2> trip_m_{};
    std::array<uint64_t, 2> trip_moving_ms_{};

    double unsaved_m_ = 0.0;
    // Set by SET / RESET: those values are absolute, not deltas to merge.
    bool odometer_overridden_ = false;
    std::array<bool, 2> trip_overridden_{};
    bool save_requested_ = false;
    bool storage_error_ = false;
    uint32_t retry_after_ms_ = 0U;

    bool have_previous_ = false;
    float previous_kph_ = 0.0f;
    uint32_t previous_ms_ = 0U;
    uint32_t stationary_since_ms_ = 0U;
    bool stationary_ = true;

    std::array<SignalValue, 3> signals_{};
};
