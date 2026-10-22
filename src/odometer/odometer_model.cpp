#include "odometer_model.h"

#include <algorithm>
#include <cmath>

namespace {
constexpr double kDecimetresPerMeter = 10.0;

std::size_t signalIndex(ParameterId id) {
    switch (id) {
        case ParameterId::Odometer: return 0U;
        case ParameterId::TripA: return 1U;
        case ParameterId::TripB: return 2U;
        default: return 3U;
    }
}

std::size_t tripIndex(TripId trip) {
    return trip == TripId::B ? 1U : 0U;
}

uint64_t toDecimetres(double meters) {
    return meters <= 0.0 ? 0U
                         : static_cast<uint64_t>(std::llround(meters * kDecimetresPerMeter));
}

const SignalValue kInvalidSignal{};
}  // namespace

OdometerModel::OdometerModel() {
    publish(0U);
}

void OdometerModel::restore(const OdometerRecord& record, uint32_t now_ms) {
    settings_.source = record.source <= static_cast<uint8_t>(DistanceSource::Gps)
                           ? static_cast<DistanceSource>(record.source)
                           : DistanceSource::CanSpeed;
    settings_.gps_fallback = record.gps_fallback != 0U;
    settings_.correction_permille = std::clamp<int16_t>(
        record.correction_permille, -kMaximumCorrectionPermille,
        kMaximumCorrectionPermille);
    odometer_m_ = static_cast<double>(record.odometer_dm) / kDecimetresPerMeter;
    for (std::size_t i = 0U; i < trip_m_.size(); ++i) {
        trip_m_[i] = static_cast<double>(record.trip_dm[i]) / kDecimetresPerMeter;
        trip_moving_ms_[i] = record.trip_moving_ms[i];
    }
    unsaved_m_ = 0.0;
    save_requested_ = false;
    odometer_overridden_ = false;
    trip_overridden_ = {};
    have_previous_ = false;
    stationary_ = true;
    stationary_since_ms_ = now_ms;
    publish(now_ms);
}

void OdometerModel::mergeStored(const OdometerRecord& record, uint32_t now_ms) {
    const OdometerSettings settings = settings_;
    const bool keep_settings = save_requested_;
    const double odometer_m = odometer_m_;
    const std::array<double, 2> trip_m = trip_m_;
    const std::array<uint64_t, 2> trip_moving_ms = trip_moving_ms_;
    const double unsaved = unsaved_m_;
    const bool odometer_overridden = odometer_overridden_;
    const std::array<bool, 2> trip_overridden = trip_overridden_;
    restore(record, now_ms);
    odometer_overridden_ = odometer_overridden;
    trip_overridden_ = trip_overridden;
    if (keep_settings) settings_ = settings;
    odometer_m_ = odometer_overridden_
        ? odometer_m
        : std::min(odometer_m_ + odometer_m, kMaximumOdometerKm * 1000.0);
    for (std::size_t i = 0U; i < trip_m_.size(); ++i) {
        trip_m_[i] = trip_overridden_[i] ? trip_m[i] : trip_m_[i] + trip_m[i];
        trip_moving_ms_[i] = trip_overridden_[i]
            ? trip_moving_ms[i] : trip_moving_ms_[i] + trip_moving_ms[i];
    }
    unsaved_m_ = unsaved;
    save_requested_ = unsaved > 0.0 || keep_settings;
    publish(now_ms);
}

OdometerRecord OdometerModel::record() const {
    OdometerRecord result{};
    result.source = static_cast<uint8_t>(settings_.source);
    result.gps_fallback = settings_.gps_fallback ? 1U : 0U;
    result.correction_permille = settings_.correction_permille;
    result.odometer_dm = toDecimetres(odometer_m_);
    for (std::size_t i = 0U; i < trip_m_.size(); ++i) {
        result.trip_dm[i] = toDecimetres(trip_m_[i]);
        result.trip_moving_ms[i] = trip_moving_ms_[i];
    }
    sealOdometerRecord(result);
    return result;
}

void OdometerModel::accumulate(double meters, uint32_t moving_ms) {
    odometer_m_ = std::min(odometer_m_ + meters, kMaximumOdometerKm * 1000.0);
    for (std::size_t i = 0U; i < trip_m_.size(); ++i) {
        trip_m_[i] += meters;
        trip_moving_ms_[i] += moving_ms;
    }
    unsaved_m_ += meters;
}

void OdometerModel::integrate(SpeedSample sample, uint32_t now_ms) {
    const bool usable = sample.valid && std::isfinite(sample.kph) &&
                        sample.kph >= 0.0f && sample.kph < 1000.0f;
    const bool moving = usable && sample.kph >= kMinimumSpeedKph;

    if (usable && have_previous_) {
        const uint32_t dt_ms = now_ms - previous_ms_;
        if (dt_ms > 0U && dt_ms <= kMaximumGapMs) {
            const float previous = previous_kph_ >= kMinimumSpeedKph ? previous_kph_ : 0.0f;
            const float current = moving ? sample.kph : 0.0f;
            if (previous > 0.0f || current > 0.0f) {
                const double average_mps =
                    (static_cast<double>(previous) + current) * 0.5 / 3.6;
                const double correction =
                    1.0 + settings_.correction_permille / 1000.0;
                accumulate(average_mps * dt_ms / 1000.0 * correction,
                           moving ? dt_ms : 0U);
                publish(now_ms);
            }
        }
    }

    if (usable) {
        have_previous_ = true;
        previous_kph_ = sample.kph;
        previous_ms_ = now_ms;
    } else {
        have_previous_ = false;
    }

    if (moving) {
        stationary_ = false;
    } else if (!stationary_) {
        stationary_ = true;
        stationary_since_ms_ = now_ms;
    }
}

bool OdometerModel::persistDue(uint32_t now_ms) const {
    if (storage_error_ && static_cast<int32_t>(now_ms - retry_after_ms_) < 0) {
        return false;
    }
    if (save_requested_) return true;
    if (unsaved_m_ >= kSaveWhileMovingM) return true;
    return unsaved_m_ > 0.0 && stationary_ &&
           now_ms - stationary_since_ms_ >= kStationarySaveMs;
}

void OdometerModel::markPersisted(uint32_t) {
    unsaved_m_ = 0.0;
    save_requested_ = false;
    storage_error_ = false;
}

void OdometerModel::persistFailed(uint32_t now_ms) {
    storage_error_ = true;
    retry_after_ms_ = now_ms + kRetryAfterFailureMs;
}

bool OdometerModel::storageError() const {
    return storage_error_;
}

bool OdometerModel::setOdometerKm(double km, uint32_t now_ms) {
    if (!std::isfinite(km) || km < 0.0 || km > kMaximumOdometerKm) {
        return false;
    }
    odometer_m_ = km * 1000.0;
    odometer_overridden_ = true;
    save_requested_ = true;
    storage_error_ = false;
    publish(now_ms);
    return true;
}

void OdometerModel::resetTrip(TripId trip, uint32_t now_ms) {
    trip_m_[tripIndex(trip)] = 0.0;
    trip_moving_ms_[tripIndex(trip)] = 0U;
    trip_overridden_[tripIndex(trip)] = true;
    save_requested_ = true;
    storage_error_ = false;
    publish(now_ms);
}

bool OdometerModel::setSettings(const OdometerSettings& settings) {
    if (static_cast<uint8_t>(settings.source) >
            static_cast<uint8_t>(DistanceSource::Gps) ||
        settings.correction_permille < -kMaximumCorrectionPermille ||
        settings.correction_permille > kMaximumCorrectionPermille) {
        return false;
    }
    settings_ = settings;
    save_requested_ = true;
    storage_error_ = false;
    return true;
}

const OdometerSettings& OdometerModel::settings() const {
    return settings_;
}

double OdometerModel::odometerKm() const {
    return odometer_m_ / 1000.0;
}

double OdometerModel::tripKm(TripId trip) const {
    return trip_m_[tripIndex(trip)] / 1000.0;
}

uint64_t OdometerModel::tripMovingMs(TripId trip) const {
    return trip_moving_ms_[tripIndex(trip)];
}

float OdometerModel::tripAverageKph(TripId trip) const {
    const uint64_t moving_ms = tripMovingMs(trip);
    if (moving_ms < 1000U) return 0.0f;
    return static_cast<float>(trip_m_[tripIndex(trip)] / 1000.0 /
                              (static_cast<double>(moving_ms) / 3600000.0));
}

double OdometerModel::unsavedMeters() const {
    return unsaved_m_;
}

const SignalValue& OdometerModel::get(ParameterId id) const {
    const std::size_t index = signalIndex(id);
    return index < signals_.size() ? signals_[index] : kInvalidSignal;
}

void OdometerModel::publish(uint32_t now_ms) {
    signals_[0] = SignalValue{static_cast<float>(odometerKm()), now_ms, true};
    signals_[1] = SignalValue{static_cast<float>(tripKm(TripId::A)), now_ms, true};
    signals_[2] = SignalValue{static_cast<float>(tripKm(TripId::B)), now_ms, true};
}
