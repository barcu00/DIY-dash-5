#include <unity.h>
#include <cstdint>
#include "alarms/buzzer_model.h"

// Catches missed startup, continuous/late warning tone, failed silence/rearm,
// accidental startup replay and millis rollover errors.
void test_startup_chirps_once_even_when_warning_sound_disabled() {
    BuzzerModel b; b.begin(1000);
    TEST_ASSERT_TRUE(b.update(1000,false,false));
    TEST_ASSERT_TRUE(b.update(1199,false,false));
    TEST_ASSERT_FALSE(b.update(1200,false,false));
    TEST_ASSERT_FALSE(b.update(2000,false,false));
}
void test_warning_pulses_and_acknowledge_rearms() {
    BuzzerModel b; b.begin(0); b.update(200,true,false);
    TEST_ASSERT_TRUE(b.update(1000,true,true));
    TEST_ASSERT_TRUE(b.update(1149,true,true));
    TEST_ASSERT_FALSE(b.update(1150,true,true));
    TEST_ASSERT_FALSE(b.update(1599,true,true));
    TEST_ASSERT_TRUE(b.update(1600,true,true));
    TEST_ASSERT_FALSE(b.update(1601,true,false));
    TEST_ASSERT_TRUE(b.update(1602,true,true));
}
void test_disable_silences_immediately_without_replaying_startup() {
    BuzzerModel b; b.begin(0); b.update(200,true,false);
    TEST_ASSERT_TRUE(b.update(1000,true,true));
    TEST_ASSERT_FALSE(b.update(1001,false,true));
    TEST_ASSERT_FALSE(b.update(1002,false,true));
    TEST_ASSERT_TRUE(b.update(1003,true,true));
    TEST_ASSERT_FALSE(b.update(1004,true,false));
}
void test_startup_and_warning_across_millis_rollover() {
    BuzzerModel b; b.begin(UINT32_MAX-99);
    TEST_ASSERT_TRUE(b.update(99,true,false));
    TEST_ASSERT_FALSE(b.update(100,true,false));
    TEST_ASSERT_TRUE(b.update(UINT32_MAX-49,true,true));
    TEST_ASSERT_FALSE(b.update(100,true,true));
    TEST_ASSERT_TRUE(b.update(550,true,true));
}
int main(int,char**) {
    UNITY_BEGIN();
    RUN_TEST(test_startup_chirps_once_even_when_warning_sound_disabled);
    RUN_TEST(test_warning_pulses_and_acknowledge_rearms);
    RUN_TEST(test_disable_silences_immediately_without_replaying_startup);
    RUN_TEST(test_startup_and_warning_across_millis_rollover);
    return UNITY_END();
}
