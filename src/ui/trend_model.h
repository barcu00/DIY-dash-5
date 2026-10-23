#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

enum class TrendDirection : uint8_t {
    Steady,
    Rising,
    RisingFast,
    Falling,
    FallingFast,
};

// Short value history for the ENDURANCE rows: a fixed number of samples
// spread over the configured window (5 .. 20 s), drawn as a sparkline
// with a direction arrow next to the value.
class TrendHistory {
public:
    static constexpr std::size_t kCapacity = 90U;

    TrendHistory() = default;
    ~TrendHistory();
    TrendHistory(const TrendHistory&) = delete;
    TrendHistory& operator=(const TrendHistory&) = delete;

    void reset();
    // Clears the history and returns its buffer to the heap.
    void release();
    bool hasStorage() const { return values_ != nullptr; }
    // Window length; the sample interval is window / capacity
    // (15 s -> 167 ms, i.e. ~6 sparkline updates per second).
    void setWindowSeconds(uint16_t seconds);
    uint32_t intervalMs() const { return interval_ms_; }
    // Returns true when a new sample was stored (the sparkline must redraw).
    bool sample(float value, bool valid, uint32_t now_ms);
    std::size_t size() const { return count_; }
    float at(std::size_t index) const;  // 0 = oldest
    // Vertical range for drawing, padded; includes `marker` when finite.
    void range(float marker, float& low, float& high) const;
    // Compares the latest samples with those a quarter window earlier, relative to the
    // given full-scale span (e.g. warning threshold or value magnitude).
    TrendDirection direction(float span) const;

private:
    // Allocated on the first sample (PSRAM on the device): only the
    // ENDURANCE rows keep a history, so idle tiles cost no internal RAM.
    float* values_ = nullptr;
    std::size_t start_ = 0U;
    std::size_t count_ = 0U;
    uint32_t last_ms_ = 0U;
    uint32_t interval_ms_ = 15000U / kCapacity;
    bool have_last_ = false;
};
