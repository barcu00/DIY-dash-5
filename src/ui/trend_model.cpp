#include "trend_model.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <limits>

#if defined(ESP_PLATFORM)
#include <esp_heap_caps.h>
#endif

namespace {
float* allocateSamples() {
    const std::size_t bytes = TrendHistory::kCapacity * sizeof(float);
#if defined(ESP_PLATFORM)
    void* memory = heap_caps_malloc(bytes, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (memory == nullptr) memory = std::malloc(bytes);
#else
    void* memory = std::malloc(bytes);
#endif
    return static_cast<float*>(memory);
}
}  // namespace

TrendHistory::~TrendHistory() {
    std::free(values_);  // heap_caps_malloc memory is released by free()
}

void TrendHistory::reset() {
    start_ = 0U;
    count_ = 0U;
    have_last_ = false;
}

void TrendHistory::release() {
    std::free(values_);
    values_ = nullptr;
    reset();
}

void TrendHistory::setWindowSeconds(uint16_t seconds) {
    const uint32_t window_ms = static_cast<uint32_t>(std::max<uint16_t>(seconds, 5U)) * 1000U;
    const uint32_t interval = std::max<uint32_t>(50U, window_ms / kCapacity);
    if (interval != interval_ms_) {
        interval_ms_ = interval;
        reset();
    }
}

bool TrendHistory::sample(float value, bool valid, uint32_t now_ms) {
    if (!valid || !std::isfinite(value)) return false;
    if (have_last_ && now_ms - last_ms_ < interval_ms_) return false;
    if (values_ == nullptr && (values_ = allocateSamples()) == nullptr) return false;
    have_last_ = true;
    last_ms_ = now_ms;
    if (count_ < kCapacity) {
        values_[(start_ + count_) % kCapacity] = value;
        ++count_;
    } else {
        values_[start_] = value;
        start_ = (start_ + 1U) % kCapacity;
    }
    return true;
}

float TrendHistory::at(std::size_t index) const {
    return index < count_ ? values_[(start_ + index) % kCapacity] : 0.0f;
}

void TrendHistory::range(float marker, float& low, float& high) const {
    low = std::numeric_limits<float>::max();
    high = std::numeric_limits<float>::lowest();
    for (std::size_t i = 0U; i < count_; ++i) {
        low = std::min(low, at(i));
        high = std::max(high, at(i));
    }
    if (count_ == 0U) { low = 0.0f; high = 1.0f; }
    if (std::isfinite(marker)) {
        low = std::min(low, marker);
        high = std::max(high, marker);
    }
    const float magnitude = std::max(std::fabs(low), std::fabs(high));
    const float minimum_span = std::max(0.5f, magnitude * 0.05f);
    if (high - low < minimum_span) {
        const float middle = (high + low) / 2.0f;
        low = middle - minimum_span / 2.0f;
        high = middle + minimum_span / 2.0f;
    }
    const float pad = (high - low) * 0.12f;
    low -= pad;
    high += pad;
}

TrendDirection TrendHistory::direction(float span) const {
    if (count_ < 3U || !(span > 0.0f)) return TrendDirection::Steady;
    constexpr std::size_t kBack = kCapacity / 4U;  // a quarter of the window
    const std::size_t newest = count_ - 1U;
    const std::size_t older = newest >= kBack ? newest - kBack : 0U;
    const float delta = at(newest) - at(older);
    const float relative = delta / span;
    if (relative > 0.08f) return TrendDirection::RisingFast;
    if (relative > 0.02f) return TrendDirection::Rising;
    if (relative < -0.08f) return TrendDirection::FallingFast;
    if (relative < -0.02f) return TrendDirection::Falling;
    return TrendDirection::Steady;
}
