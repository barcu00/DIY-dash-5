#include <cstddef>
#include <cstring>

#include <unity.h>

#include "racechrono/racechrono_channel_catalog.h"
#include "telemetry/parameter_registry.h"

void test_catalog_contains_all_unique_monitor_channels() {
    TEST_ASSERT_EQUAL_UINT32(33U, raceChronoChannelCount());

    bool seen[256]{};
    for (std::size_t index = 0U; index < raceChronoChannelCount(); ++index) {
        const RaceChronoChannelDescriptor& channel = raceChronoChannelAt(index);
        TEST_ASSERT_TRUE(isRaceChronoParameter(channel.parameter));
        TEST_ASSERT_NOT_EQUAL(0U, channel.monitor_id);
        TEST_ASSERT_FALSE(seen[channel.monitor_id]);
        seen[channel.monitor_id] = true;
        TEST_ASSERT_EQUAL_UINT32(5000U, channel.stale_ms);
        TEST_ASSERT_EQUAL_PTR(&channel, raceChronoChannel(channel.parameter));
        TEST_ASSERT_EQUAL_PTR(&channel,
                              raceChronoChannelByMonitorId(channel.monitor_id));
    }
}

void test_catalog_pins_equations_scales_and_coordinate_encoding() {
    const auto* gps_speed = raceChronoChannel(ParameterId::RcGpsSpeed);
    const auto* delta_speed = raceChronoChannel(ParameterId::RcDeltaSpeed);
    const auto* latitude = raceChronoChannel(ParameterId::RcLatitude);
    const auto* longitude = raceChronoChannel(ParameterId::RcLongitude);

    TEST_ASSERT_NOT_NULL(gps_speed);
    TEST_ASSERT_EQUAL_UINT8(17U, gps_speed->monitor_id);
    TEST_ASSERT_EQUAL_STRING("channel(device(gps), speed) * 360",
                             gps_speed->equation);
    TEST_ASSERT_FLOAT_WITHIN(0.00001f, 0.01f, gps_speed->raw_to_native);

    TEST_ASSERT_NOT_NULL(delta_speed);
    TEST_ASSERT_EQUAL_UINT8(29U, delta_speed->monitor_id);
    TEST_ASSERT_EQUAL_STRING("channel(device(calc), delta_speed) * 360",
                             delta_speed->equation);
    TEST_ASSERT_FLOAT_WITHIN(0.00001f, 0.01f, delta_speed->raw_to_native);

    TEST_ASSERT_NOT_NULL(latitude);
    TEST_ASSERT_NOT_NULL(longitude);
    TEST_ASSERT_EQUAL_UINT8(
        static_cast<uint8_t>(RaceChronoValueEncoding::CoordinateDegreesTimes6000000),
        static_cast<uint8_t>(latitude->encoding));
    TEST_ASSERT_EQUAL_UINT8(
        static_cast<uint8_t>(RaceChronoValueEncoding::CoordinateDegreesTimes6000000),
        static_cast<uint8_t>(longitude->encoding));
}

void test_catalog_parameter_ids_are_appended_in_monitor_order() {
    TEST_ASSERT_EQUAL_UINT8(
        static_cast<uint8_t>(ParameterId::CruiseControlActive) + 1U,
        static_cast<uint8_t>(ParameterId::RcLapNumber));
    TEST_ASSERT_EQUAL_UINT8(
        static_cast<uint8_t>(ParameterId::RcLapNumber) + 32U,
        static_cast<uint8_t>(ParameterId::RcLeanAngle));

    for (std::size_t index = 0U; index < raceChronoChannelCount(); ++index) {
        TEST_ASSERT_EQUAL_UINT8(
            static_cast<uint8_t>(ParameterId::RcLapNumber) + index,
            static_cast<uint8_t>(raceChronoChannelAt(index).parameter));
        TEST_ASSERT_EQUAL_UINT8(index + 1U,
                                raceChronoChannelAt(index).monitor_id);
    }
}

void test_comparison_lap_time_uses_approved_picker_label() {
    const ParameterDescriptor& descriptor =
        parameterDescriptor(ParameterId::RcComparisonLapTime);
    TEST_ASSERT_EQUAL_STRING("THEORETICAL / REFERENCE", descriptor.short_name);
    TEST_ASSERT_EQUAL_UINT8(3U, descriptor.default_decimals);
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_catalog_contains_all_unique_monitor_channels);
    RUN_TEST(test_catalog_pins_equations_scales_and_coordinate_encoding);
    RUN_TEST(test_catalog_parameter_ids_are_appended_in_monitor_order);
    RUN_TEST(test_comparison_lap_time_uses_approved_picker_label);
    return UNITY_END();
}
