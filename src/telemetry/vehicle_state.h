#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

enum class DataSource : uint8_t {
    None,
    Can,
    Demo,
};

enum class VehicleSignal : uint8_t {
    Rpm,
    Map,
    Lambda,
    Tps,
    Clt,
    Iat,
    OilPressure,
    OilTemperature,
    BatteryVoltage,
    Speed,
    Gear,
    FuelPressure,
    Count,
};

struct SignalValue {
    float value = 0.0f;
    uint32_t updated_ms = 0U;
    bool valid = false;
};

class VehicleState {
public:
    static constexpr std::size_t kSignalCount =
        static_cast<std::size_t>(VehicleSignal::Count);

    void reset(DataSource source = DataSource::None) {
        values_.fill(SignalValue{});
        source_ = source;
    }

    DataSource source() const {
        return source_;
    }

    void set(VehicleSignal signal, float value, uint32_t now_ms) {
        values_[index(signal)] = SignalValue{value, now_ms, true};
    }

    const SignalValue& get(VehicleSignal signal) const {
        return values_[index(signal)];
    }

    void invalidate(VehicleSignal signal) {
        values_[index(signal)].valid = false;
    }

    void invalidateStale(uint32_t now_ms, const uint32_t* timeouts_ms) {
        if (timeouts_ms == nullptr) {
            return;
        }
        for (std::size_t i = 0; i < values_.size(); ++i) {
            if (values_[i].valid && timeouts_ms[i] > 0U &&
                now_ms - values_[i].updated_ms > timeouts_ms[i]) {
                values_[i].valid = false;
            }
        }
    }

private:
    static constexpr std::size_t index(VehicleSignal signal) {
        return static_cast<std::size_t>(signal);
    }

    std::array<SignalValue, kSignalCount> values_{};
    DataSource source_ = DataSource::None;
};
