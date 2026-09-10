#include <unity.h>

#include "ui/frame_scheduler.h"

void test_shift_and_render_have_independent_deadlines() {
    FrameScheduler scheduler;
    scheduler.reset(1000U);

    TEST_ASSERT_FALSE(scheduler.takeShift(1004U));
    TEST_ASSERT_TRUE(scheduler.takeShift(1005U));
    TEST_ASSERT_FALSE(scheduler.takeRender(1024U));
    TEST_ASSERT_TRUE(scheduler.takeRender(1025U));
}

void test_late_render_skips_missed_periods_without_drift_or_burst() {
    FrameScheduler scheduler;
    scheduler.reset(1000U);

    TEST_ASSERT_TRUE(scheduler.takeRender(1079U));
    TEST_ASSERT_FALSE(scheduler.takeRender(1080U));
    TEST_ASSERT_FALSE(scheduler.takeRender(1099U));
    TEST_ASSERT_TRUE(scheduler.takeRender(1100U));
}

void test_late_shift_skips_missed_periods_without_drift_or_burst() {
    FrameScheduler scheduler;
    scheduler.reset(1000U);

    TEST_ASSERT_TRUE(scheduler.takeShift(1018U));
    TEST_ASSERT_FALSE(scheduler.takeShift(1019U));
    TEST_ASSERT_TRUE(scheduler.takeShift(1020U));
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_shift_and_render_have_independent_deadlines);
    RUN_TEST(test_late_render_skips_missed_periods_without_drift_or_burst);
    RUN_TEST(test_late_shift_skips_missed_periods_without_drift_or_burst);
    return UNITY_END();
}
