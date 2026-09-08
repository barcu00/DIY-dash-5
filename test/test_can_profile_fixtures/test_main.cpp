#include <unity.h>

#include "ecu/can_profile_registry.h"
#include "ecu/ecu_can_decoder.h"

namespace {
void assertValue(const VehicleState& state, ParameterId parameter,
                 float expected, float tolerance = 0.01f) {
    const SignalValue& value = state.get(parameter);
    TEST_ASSERT_TRUE(value.valid);
    TEST_ASSERT_FLOAT_WITHIN(tolerance, expected, value.value);
}

VehicleState decode(const char* profile_id, const CanFrame& frame) {
    EcuCanDecoder decoder(CanProfileRegistry::find(profile_id));
    VehicleState state;
    state.reset(DataSource::Can);
    TEST_ASSERT_TRUE(decoder.decode(frame, state, 123U));
    return state;
}
}  // namespace

void test_ecumaster_core_frames_decode_to_native_units() {
    VehicleState first = decode(
        "ecumaster_emu_black",
        CanFrame{0x600U, 8U,
                 {0x40U, 0x1FU, 100U, 25U, 0xFAU, 0x00U, 0U, 0U},
                 false, false});
    assertValue(first, ParameterId::Rpm, 8000.0f);
    assertValue(first, ParameterId::Tps, 50.0f);
    assertValue(first, ParameterId::Iat, 25.0f);
    assertValue(first, ParameterId::Map, 2.5f);

    VehicleState second = decode(
        "ecumaster_emu_black",
        CanFrame{0x602U, 8U,
                 {123U, 0U, 100U, 100U, 64U, 80U, 90U, 0U},
                 false, false});
    assertValue(second, ParameterId::Speed, 123.0f);
    assertValue(second, ParameterId::OilTemperature, 100.0f);
    assertValue(second, ParameterId::OilPressure, 4.0f);
    assertValue(second, ParameterId::FuelPressure, 5.0f);
    assertValue(second, ParameterId::Clt, 90.0f);

    VehicleState lambda = decode(
        "ecumaster_emu_black",
        CanFrame{0x603U, 8U, {0U, 0U, 128U, 0U, 0U, 0U, 0U, 0U},
                 false, false});
    assertValue(lambda, ParameterId::Lambda, 1.0f);

    VehicleState status = decode(
        "ecumaster_emu_black",
        CanFrame{0x604U, 8U, {3U, 0U, 0xF4U, 0x01U, 0U, 0U, 0U, 0U},
                 false, false});
    assertValue(status, ParameterId::Gear, 3.0f);
    assertValue(status, ParameterId::BatteryVoltage, 13.5f);
}

void test_rusefi_core_frames_decode_to_native_units() {
    VehicleState speeds = decode(
        "rusefi_verbose",
        CanFrame{0x201U, 8U, {0x94U, 0x11U, 0U, 0U, 0U, 0U, 123U, 0U},
                 false, false});
    assertValue(speeds, ParameterId::Rpm, 4500.0f);
    assertValue(speeds, ParameterId::Speed, 123.0f);

    VehicleState sensors = decode(
        "rusefi_verbose",
        CanFrame{0x203U, 8U, {0xB8U, 0x0BU, 130U, 70U, 0U, 0U, 0U, 0U},
                 false, false});
    assertValue(sensors, ParameterId::Map, 1.0f, 0.001f);
    assertValue(sensors, ParameterId::Clt, 90.0f);
    assertValue(sensors, ParameterId::Iat, 30.0f);

    VehicleState fueling = decode(
        "rusefi_verbose",
        CanFrame{0x207U, 8U,
                 {0x10U, 0x27U, 0U, 0U, 0xB8U, 0x0BU, 0U, 0U},
                 false, false});
    assertValue(fueling, ParameterId::Lambda, 1.0f);
    assertValue(fueling, ParameterId::FuelPressure, 1.0f, 0.001f);
}

void test_maxxecu_core_frames_decode_to_native_units() {
    VehicleState fast = decode(
        "maxxecu_default_1_3",
        CanFrame{0x520U, 8U,
                 {0xA0U, 0x0FU, 0xF4U, 0x01U, 0xDCU, 0x05U, 0xE8U, 0x03U},
                 false, false});
    assertValue(fast, ParameterId::Rpm, 4000.0f);
    assertValue(fast, ParameterId::Tps, 50.0f);
    assertValue(fast, ParameterId::Map, 1.5f);
    assertValue(fast, ParameterId::Lambda, 1.0f);

    VehicleState slow = decode(
        "maxxecu_default_1_3",
        CanFrame{0x536U, 8U,
                 {3U, 0U, 0U, 0U, 0xA0U, 0x0FU, 0xE8U, 0x03U},
                 false, false});
    assertValue(slow, ParameterId::Gear, 3.0f);
    assertValue(slow, ParameterId::OilPressure, 4.0f);
    assertValue(slow, ParameterId::OilTemperature, 100.0f);
}

void test_haltech_and_speeduino_compatible_frames_decode_big_endian() {
    const CanFrame data1{0x360U, 8U,
                         {0x0FU, 0xA0U, 0x05U, 0xDCU,
                          0x01U, 0xF4U, 0U, 0U},
                         false, false};
    VehicleState haltech = decode("haltech_broadcast_2_0", data1);
    VehicleState speeduino = decode("speeduino_haltech", data1);
    for (const VehicleState* state : {&haltech, &speeduino}) {
        assertValue(*state, ParameterId::Rpm, 4000.0f);
        assertValue(*state, ParameterId::Map, 1.5f);
        assertValue(*state, ParameterId::Tps, 50.0f);
    }

    VehicleState lambda = decode(
        "speeduino_haltech",
        CanFrame{0x368U, 8U, {0x03U, 0xE8U, 0U, 0U, 0U, 0U, 0U, 0U},
                 false, false});
    assertValue(lambda, ParameterId::Lambda, 1.0f);
}

void test_link_indexed_frames_require_the_discriminator() {
    VehicleState rpm = decode(
        "link_generic_dash_experimental",
        CanFrame{0x3E8U, 8U, {0U, 0U, 0x94U, 0x11U, 0U, 0U, 0U, 0U},
                 false, false});
    assertValue(rpm, ParameterId::Rpm, 4500.0f);

    EcuCanDecoder decoder(
        CanProfileRegistry::find("link_generic_dash_experimental"));
    VehicleState rejected;
    rejected.reset(DataSource::Can);
    const CanFrame wrong_reserved{
        0x3E8U, 8U, {0U, 1U, 0x94U, 0x11U, 0U, 0U, 0U, 0U}, false, false};
    TEST_ASSERT_FALSE(decoder.decode(wrong_reserved, rejected, 1U));
    TEST_ASSERT_FALSE(rejected.get(ParameterId::Rpm).valid);
}

void test_psa_profile_decodes_only_corroborated_engine_sensors() {
    VehicleState fast = decode(
        "psa_c2_vts_engine_experimental",
        CanFrame{0x208U, 8U, {0xA0U, 0x8CU, 0U, 100U, 0U, 0U, 0U, 0U},
                 false, false});
    assertValue(fast, ParameterId::Rpm, 4500.0f);
    assertValue(fast, ParameterId::Tps, 50.0f);

    VehicleState slow = decode(
        "psa_c2_vts_engine_experimental",
        CanFrame{0x488U, 8U, {130U, 0U, 0U, 0U, 0U, 140U, 70U, 80U},
                 false, false});
    assertValue(slow, ParameterId::Clt, 90.0f);
    assertValue(slow, ParameterId::OilTemperature, 100.0f);
    assertValue(slow, ParameterId::Iat, 40.0f);
    TEST_ASSERT_FALSE(slow.get(ParameterId::Gear).valid);
}

void test_wrong_dlc_and_format_are_rejected_without_partial_update() {
    EcuCanDecoder decoder(CanProfileRegistry::find("ecumaster_emu_black"));
    VehicleState state;
    state.reset(DataSource::Can);
    state.set(ParameterId::Gear, 4.0f, 1U);

    const CanFrame short_frame{
        0x600U, 7U, {0x40U, 0x1FU, 100U, 25U, 0xFAU, 0U, 0U}, false, false};
    TEST_ASSERT_FALSE(decoder.decode(short_frame, state, 2U));
    TEST_ASSERT_FALSE(state.get(ParameterId::Rpm).valid);
    assertValue(state, ParameterId::Gear, 4.0f);

    CanFrame extended = short_frame;
    extended.dlc = 8U;
    extended.extended = true;
    TEST_ASSERT_FALSE(decoder.decode(extended, state, 3U));

    extended.extended = false;
    extended.remote = true;
    TEST_ASSERT_FALSE(decoder.decode(extended, state, 4U));
}

void test_out_of_range_signal_rejects_the_whole_matching_frame() {
    EcuCanDecoder decoder(CanProfileRegistry::find("ecumaster_emu_black"));
    VehicleState state;
    state.reset(DataSource::Can);

    const CanFrame invalid_rpm{
        0x600U, 8U, {0xFFU, 0xFFU, 100U, 25U, 0xFAU, 0U, 0U, 0U},
        false, false};
    TEST_ASSERT_FALSE(decoder.decode(invalid_rpm, state, 5U));
    TEST_ASSERT_FALSE(state.get(ParameterId::Rpm).valid);
    TEST_ASSERT_FALSE(state.get(ParameterId::Tps).valid);
    TEST_ASSERT_FALSE(state.get(ParameterId::Iat).valid);
    TEST_ASSERT_FALSE(state.get(ParameterId::Map).valid);
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_ecumaster_core_frames_decode_to_native_units);
    RUN_TEST(test_rusefi_core_frames_decode_to_native_units);
    RUN_TEST(test_maxxecu_core_frames_decode_to_native_units);
    RUN_TEST(test_haltech_and_speeduino_compatible_frames_decode_big_endian);
    RUN_TEST(test_link_indexed_frames_require_the_discriminator);
    RUN_TEST(test_psa_profile_decodes_only_corroborated_engine_sensors);
    RUN_TEST(test_wrong_dlc_and_format_are_rejected_without_partial_update);
    RUN_TEST(test_out_of_range_signal_rejects_the_whole_matching_frame);
    return UNITY_END();
}
