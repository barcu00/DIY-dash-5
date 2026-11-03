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
    TEST_ASSERT_FALSE(history.sample(2.0f, false, now + history.intervalMs()));
    TEST_ASSERT_EQUAL_UINT32(166U, history.intervalMs());  // 15 s default
    for (int i = 1; i < 120; ++i) {
        now += history.intervalMs();
        TEST_ASSERT_TRUE(history.sample(static_cast<float>(i + 1), true, now));
    }
    TEST_ASSERT_EQUAL_UINT32(TrendHistory::kCapacity, history.size());
    TEST_ASSERT_EQUAL_FLOAT(120.0f - TrendHistory::kCapacity + 1.0f, history.at(0));
    TEST_ASSERT_EQUAL_FLOAT(120.0f, history.at(TrendHistory::kCapacity - 1U));
    history.setWindowSeconds(20U);
    TEST_ASSERT_EQUAL_UINT32(222U, history.intervalMs());
    TEST_ASSERT_EQUAL_UINT32(0U, history.size());
}

void test_release_frees_buffer_until_next_sample() {
    TrendHistory history;
    TEST_ASSERT_FALSE(history.hasStorage());
    TEST_ASSERT_TRUE(history.sample(1.0f, true, 0U));
    TEST_ASSERT_TRUE(history.hasStorage());
    history.release();
    TEST_ASSERT_FALSE(history.hasStorage());
    TEST_ASSERT_EQUAL_UINT32(0U, history.size());
    TEST_ASSERT_TRUE(history.sample(2.0f, true, 1U));
    TEST_ASSERT_EQUAL_FLOAT(2.0f, history.at(0));
}

void test_range_includes_threshold_and_has_minimum_span() {
    TrendHistory history;
    for (int i = 0; i < 5; ++i) history.sample(98.0f, true, i * 1000U);
    float low = 0.0f, high = 0.0f;
    history.range(NAN, low, high);
    TEST_ASSERT_TRUE(high - low >= 4.0f);
    TEST_ASSERT_TRUE(low < 98.0f && high > 98.0f);
    history.range(125.0f, low, high);
    TEST_ASSERT_TRUE(high > 125.0f);
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_release_frees_buffer_until_next_sample);
    RUN_TEST(test_samples_once_per_interval_and_keeps_order_when_full);
    RUN_TEST(test_range_includes_threshold_and_has_minimum_span);
    return UNITY_END();
}
