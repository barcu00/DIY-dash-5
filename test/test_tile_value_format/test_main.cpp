#include <unity.h>

// Production changes caught: lap times shown as raw seconds, deltas losing
// their sign, or the colon reaching tiles that are not times.

#include "ui/tile_value_format.h"

void test_lap_times_use_minutes() {
    char text[32];
    formatTileValue(ParameterId::RcLapTime, 102.38f, 2U, text, sizeof(text));
    TEST_ASSERT_EQUAL_STRING("1:42.38", text);
    formatTileValue(ParameterId::RcTotalRaceTime, 9068.0f, 0U, text, sizeof(text));
    TEST_ASSERT_EQUAL_STRING("2:31:08", text);
    formatTileValue(ParameterId::RcLapTime, 59.4f, 1U, text, sizeof(text));
    TEST_ASSERT_EQUAL_STRING("59.4", text);
}

void test_deltas_are_signed_and_toned() {
    char text[32];
    formatTileValue(ParameterId::RcDeltaLapTime, -0.42f, 2U, text, sizeof(text));
    TEST_ASSERT_EQUAL_STRING("-0.42", text);
    formatTileValue(ParameterId::RcDeltaLapTime, 0.31f, 2U, text, sizeof(text));
    TEST_ASSERT_EQUAL_STRING("+0.31", text);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(DeltaTone::Better),
        static_cast<uint8_t>(deltaTone(ParameterId::RcDeltaLapTime, -0.1f)));
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(DeltaTone::Worse),
        static_cast<uint8_t>(deltaTone(ParameterId::RcDeltaLapTime, 0.1f)));
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(DeltaTone::Neutral),
        static_cast<uint8_t>(deltaTone(ParameterId::Clt, -5.0f)));
}

void test_other_values_keep_plain_decimals() {
    char text[32];
    formatTileValue(ParameterId::Clt, 92.4f, 0U, text, sizeof(text));
    TEST_ASSERT_EQUAL_STRING("92", text);
    formatTileValue(ParameterId::Odometer, 184532.4f, 1U, text, sizeof(text));
    TEST_ASSERT_EQUAL_STRING("184532.4", text);
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_lap_times_use_minutes);
    RUN_TEST(test_deltas_are_signed_and_toned);
    RUN_TEST(test_other_values_keep_plain_decimals);
    return UNITY_END();
}
