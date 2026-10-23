#include <cmath>

#include <unity.h>

// Production changes caught: samples taken faster than the interval, the
// ring buffer losing order when full, flat traces drawn as noise, the
// warning threshold falling outside the drawn range, wrong arrows.

#include "ui/trend_model.h"

void test_samples_once_per_interval_and_keeps_order_when_full() {
    TrendHistory history;
    uint32_t now = 0U;
    TEST_ASSERT_TRUE(history.sample(1.0f, true, now));
    TEST_ASSERT_FALSE(history.sample(2.0f, true, now + 100U));
    TEST_ASSERT_FALSE(history.sample(2.0f, false, now + TrendHistory::kIntervalMs));
    for (int i = 1; i < 60; ++i) {
        now += TrendHistory::kIntervalMs;
        TEST_ASSERT_TRUE(history.sample(static_cast<float>(i + 1), true, now));
    }
    TEST_ASSERT_EQUAL_UINT32(TrendHistory::kCapacity, history.size());
    TEST_ASSERT_EQUAL_FLOAT(60.0f - TrendHistory::kCapacity + 1.0f, history.at(0));
    TEST_ASSERT_EQUAL_FLOAT(60.0f, history.at(TrendHistory::kCapacity - 1U));
}

void test_range_includes_threshold_and_has_minimum_span() {
    TrendHistory history;
    for (int i = 0; i < 5; ++i) history.sample(98.0f, true, i * TrendHistory::kIntervalMs);
    float low = 0.0f, high = 0.0f;
    history.range(NAN, low, high);
    TEST_ASSERT_TRUE(high - low >= 4.0f);
    TEST_ASSERT_TRUE(low < 98.0f && high > 98.0f);
    history.range(125.0f, low, high);
    TEST_ASSERT_TRUE(high > 125.0f);
}

void test_direction_categories() {
    TrendHistory rising;
    TrendHistory steady;
    TrendHistory falling;
    for (int i = 0; i < 20; ++i) {
        const uint32_t t = i * TrendHistory::kIntervalMs;
        rising.sample(100.0f + i * 1.5f, true, t);
        steady.sample(3.9f + ((i % 2) ? 0.01f : 0.0f), true, t);
        falling.sample(40.0f - i * 0.2f, true, t);
    }
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(TrendDirection::RisingFast),
                            static_cast<uint8_t>(rising.direction(130.0f)));
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(TrendDirection::Steady),
                            static_cast<uint8_t>(steady.direction(10.0f)));
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(TrendDirection::Falling),
                            static_cast<uint8_t>(falling.direction(60.0f)));
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_samples_once_per_interval_and_keeps_order_when_full);
    RUN_TEST(test_range_includes_threshold_and_has_minimum_span);
    RUN_TEST(test_direction_categories);
    return UNITY_END();
}
