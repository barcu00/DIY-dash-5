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

// Short value history for the ENDURANCE rows: one sample per interval,
// drawn as a sparkline with a direction arrow next to the value.
class TrendHistory {
public:
    static constexpr std::size_t kCapacity = 48U;
    static constexpr uint32_t kIntervalMs = 2500U;  // 2 minutes of history

    void reset();
    // Returns true when a new sample was stored (the sparkline must redraw).
    bool sample(float value, bool valid, uint32_t now_ms);
    std::size_t size() const { return count_; }
    float at(std::size_t index) const;  // 0 = oldest
    // Vertical range for drawing, padded; includes `marker` when finite.
    void range(float marker, float& low, float& high) const;
    // Compares the latest samples with those ~30 s earlier, relative to the
    // given full-scale span (e.g. warning threshold or value magnitude).
    TrendDirection direction(float span) const;

private:
    std::array<float, kCapacity> values_{};
    std::size_t start_ = 0U;
    std::size_t count_ = 0U;
    uint32_t last_ms_ = 0U;
    bool have_last_ = false;
};
