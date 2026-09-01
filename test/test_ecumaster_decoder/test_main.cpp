#include <unity.h>

#include "ecu/ecumaster_decoder.h"
#include "telemetry/parameter_registry.h"

namespace {
void assertValue(const ParameterRegistry& r, ParameterId id, float expected, float tolerance = 0.001f) {
    TEST_ASSERT_TRUE(r.value(id).valid);
    TEST_ASSERT_FLOAT_WITHIN(tolerance, expected, r.value(id).value);
}

void test_frame_600_engine_values() {
    ParameterRegistry r;
    EcumasterDecoder d(r);
    const uint8_t data[8] = {0xE1, 0x10, 150, 0xF4, 0xB9, 0x00, 0x36, 0x01};

    TEST_ASSERT_TRUE(d.decode(0x600, data, 8, 100));
    assertValue(r, ParameterId::Rpm, 4321.0f);
    assertValue(r, ParameterId::Tps, 75.0f);
    assertValue(r, ParameterId::Iat, -12.0f);
    assertValue(r, ParameterId::Map, 1.85f);
    assertValue(r, ParameterId::InjectorPulseWidth, 310.0f / 62.0f, 0.0001f);
}

void test_frame_601_analog_inputs() {
    ParameterRegistry r;
    EcumasterDecoder d(r);
    const uint8_t data[8] = {0x00,0x02, 0x00,0x01, 0x00,0x03, 0x00,0x04};

    TEST_ASSERT_TRUE(d.decode(0x601, data, 8, 200));
    assertValue(r, ParameterId::Ain1, 2.5f);
    assertValue(r, ParameterId::Ain2, 1.25f);
    assertValue(r, ParameterId::Ain3, 3.75f);
    assertValue(r, ParameterId::Ain4, 5.0f);
}

void test_frame_602_vehicle_pressures_and_signed_clt() {
    ParameterRegistry r;
    EcumasterDecoder d(r);
    const uint8_t data[8] = {123,0, 100,95, 72,64, 0xD8,0xFF};

    TEST_ASSERT_TRUE(d.decode(0x602, data, 8, 300));
    assertValue(r, ParameterId::VehicleSpeed, 123.0f);
    assertValue(r, ParameterId::BarometricPressure, 1.0f);
    assertValue(r, ParameterId::OilTemperature, 95.0f);
    assertValue(r, ParameterId::OilPressure, 4.5f);
    assertValue(r, ParameterId::FuelPressure, 4.0f);
    assertValue(r, ParameterId::Clt, -40.0f);
}

void test_frame_603_ignition_lambda_and_egt() {
    ParameterRegistry r;
    EcumasterDecoder d(r);
    const uint8_t data[8] = {30,70,128,200, 0x0C,0x03, 0x2A,0x03};

    TEST_ASSERT_TRUE(d.decode(0x603, data, 8, 400));
    assertValue(r, ParameterId::IgnitionAngle, 15.0f);
    assertValue(r, ParameterId::Dwell, 3.5f);
    assertValue(r, ParameterId::Lambda, 1.0f);
    assertValue(r, ParameterId::LambdaCorrection, 100.0f);
    assertValue(r, ParameterId::Egt1, 780.0f);
    assertValue(r, ParameterId::Egt2, 810.0f);
}

void test_frame_604_errors_flags_and_ethanol() {
    ParameterRegistry r;
    EcumasterDecoder d(r);
    // Error bits: ERR_CLT (0), ERR_DBW (9). FLAGS1: GEARCUT, LC, TC, BRAKE.
    const uint8_t data[8] = {3,55, 0xF4,0x01, 0x01,0x02, 0xA5,85};

    TEST_ASSERT_TRUE(d.decode(0x604, data, 8, 500));
    assertValue(r, ParameterId::Gear, 3.0f);
    assertValue(r, ParameterId::EcuTemperature, 55.0f);
    assertValue(r, ParameterId::BatteryVoltage, 13.5f);
    assertValue(r, ParameterId::EthanolContent, 85.0f);
    assertValue(r, ParameterId::EcuError, 1.0f);
    assertValue(r, ParameterId::CltSensorError, 1.0f);
    assertValue(r, ParameterId::IatSensorError, 0.0f);
    assertValue(r, ParameterId::DbwError, 1.0f);
    assertValue(r, ParameterId::GearCutActive, 1.0f);
    assertValue(r, ParameterId::LaunchControlActive, 1.0f);
    assertValue(r, ParameterId::TractionControlActive, 1.0f);
    assertValue(r, ParameterId::BrakeActive, 1.0f);
    assertValue(r, ParameterId::AntiLagActive, 0.0f);
    assertValue(r, ParameterId::IdleActive, 0.0f);
    assertValue(r, ParameterId::TableSet2Active, 0.0f);
    assertValue(r, ParameterId::PitLimiterActive, 0.0f);
}

void test_frame_604_all_documented_error_bits_are_exposed() {
    ParameterRegistry r;
    EcumasterDecoder d(r);
    const uint8_t data[8] = {0,0, 0,0, 0xFF,0x07, 0,0};

    TEST_ASSERT_TRUE(d.decode(0x604, data, 8, 501));
    assertValue(r, ParameterId::CltSensorError, 1);
    assertValue(r, ParameterId::IatSensorError, 1);
    assertValue(r, ParameterId::MapSensorError, 1);
    assertValue(r, ParameterId::WidebandError, 1);
    assertValue(r, ParameterId::Egt1SensorError, 1);
    assertValue(r, ParameterId::Egt2SensorError, 1);
    assertValue(r, ParameterId::EgtAlarm, 1);
    assertValue(r, ParameterId::Knocking, 1);
    assertValue(r, ParameterId::FlexFuelSensorError, 1);
    assertValue(r, ParameterId::DbwError, 1);
    assertValue(r, ParameterId::FuelPressureRelativeError, 1);
}

void test_frame_605_dbw_and_tc_signed_raw() {
    ParameterRegistry r;
    EcumasterDecoder d(r);
    const uint8_t data[8] = {100,120, 0xE7,0xFF, 150,0, 30,20};

    TEST_ASSERT_TRUE(d.decode(0x605, data, 8, 600));
    assertValue(r, ParameterId::DbwPosition, 50.0f);
    assertValue(r, ParameterId::DbwTarget, 60.0f);
    assertValue(r, ParameterId::TcDifferentialRpmRaw, -25.0f);
    assertValue(r, ParameterId::TcDifferentialRpmFiltered, 150.0f);
    assertValue(r, ParameterId::TcTorqueReduction, 30.0f);
    assertValue(r, ParameterId::PitLimiterTorqueReduction, 20.0f);
}

void test_frame_606_analog_and_all_output_flag_groups() {
    ParameterRegistry r;
    EcumasterDecoder d(r);
    // OUT1 PO1+VPO1, OUT2 CANSW1+CANSW5, OUT3 SW1+MUXSW1+LCMAP+ALSMAP,
    // OUT4 fuel pump+fan+nitrous+starter+boost map.
    const uint8_t data[8] = {0x00,0x02, 0x00,0x01, 0x21,0x11,0xC9,0x73};

    TEST_ASSERT_TRUE(d.decode(0x606, data, 8, 700));
    assertValue(r, ParameterId::Ain5, 2.5f);
    assertValue(r, ParameterId::Ain6, 1.25f);
    assertValue(r, ParameterId::ParametricOutput1, 1);
    assertValue(r, ParameterId::ParametricOutput5, 0);
    assertValue(r, ParameterId::VirtualOutput1, 1);
    assertValue(r, ParameterId::VirtualOutput3, 0);
    assertValue(r, ParameterId::CanSwitch1, 1);
    assertValue(r, ParameterId::CanSwitch5, 1);
    assertValue(r, ParameterId::CanSwitch8, 0);
    assertValue(r, ParameterId::Switch1, 1);
    assertValue(r, ParameterId::Switch2, 0);
    assertValue(r, ParameterId::MuxSwitch1, 1);
    assertValue(r, ParameterId::LaunchMapSet, 1);
    assertValue(r, ParameterId::AlsMapSet, 1);
    assertValue(r, ParameterId::FuelPumpActive, 1);
    assertValue(r, ParameterId::FanActive, 1);
    assertValue(r, ParameterId::AcClutchActive, 0);
    assertValue(r, ParameterId::AcFanActive, 0);
    assertValue(r, ParameterId::NitrousActive, 1);
    assertValue(r, ParameterId::StarterRequest, 1);
    assertValue(r, ParameterId::BoostMapSet, 1);
}

void test_frame_606_can_switches_1_through_8() {
    ParameterRegistry r;
    EcumasterDecoder d(r);
    const uint8_t data[8] = {0,0,0,0, 0,0xFF,0,0};
    TEST_ASSERT_TRUE(d.decode(0x606, data, 8, 701));
    assertValue(r, ParameterId::CanSwitch1, 1);
    assertValue(r, ParameterId::CanSwitch2, 1);
    assertValue(r, ParameterId::CanSwitch3, 1);
    assertValue(r, ParameterId::CanSwitch4, 1);
    assertValue(r, ParameterId::CanSwitch5, 1);
    assertValue(r, ParameterId::CanSwitch6, 1);
    assertValue(r, ParameterId::CanSwitch7, 1);
    assertValue(r, ParameterId::CanSwitch8, 1);
}

void test_frame_607_targets_pwm_dsg_and_fuel_used() {
    ParameterRegistry r;
    EcumasterDecoder d(r);
    const uint8_t data[8] = {0xB9,0x00, 40,6, 92,55, 0xD2,0x04};

    TEST_ASSERT_TRUE(d.decode(0x607, data, 8, 800));
    assertValue(r, ParameterId::BoostTarget, 1.85f);
    assertValue(r, ParameterId::Pwm1, 40.0f);
    assertValue(r, ParameterId::DsgMode, 6.0f);
    assertValue(r, ParameterId::LambdaTarget, 0.92f);
    assertValue(r, ParameterId::Pwm2, 55.0f);
    assertValue(r, ParameterId::FuelUsed, 12.34f);
}

void test_decoder_rejects_bad_id_and_dlc_without_updates() {
    ParameterRegistry r;
    EcumasterDecoder d(r);
    const uint8_t data[8] = {0};

    TEST_ASSERT_FALSE(d.decode(0x5FF, data, 8, 900));
    TEST_ASSERT_FALSE(d.decode(0x600, data, 7, 900));
    TEST_ASSERT_FALSE(r.value(ParameterId::Rpm).valid);
}

void test_base_id_can_be_relocated() {
    ParameterRegistry r;
    EcumasterDecoder d(r);
    d.setBaseId(0x700);
    const uint8_t data[8] = {0xD2,0x04,0,0,0,0,0,0};

    TEST_ASSERT_FALSE(d.decode(0x600, data, 8, 1000));
    TEST_ASSERT_TRUE(d.decode(0x700, data, 8, 1000));
    assertValue(r, ParameterId::Rpm, 1234.0f);
    TEST_ASSERT_EQUAL_UINT32(1000U, r.value(ParameterId::Rpm).updated_ms);
}
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_frame_600_engine_values);
    RUN_TEST(test_frame_601_analog_inputs);
    RUN_TEST(test_frame_602_vehicle_pressures_and_signed_clt);
    RUN_TEST(test_frame_603_ignition_lambda_and_egt);
    RUN_TEST(test_frame_604_errors_flags_and_ethanol);
    RUN_TEST(test_frame_604_all_documented_error_bits_are_exposed);
    RUN_TEST(test_frame_605_dbw_and_tc_signed_raw);
    RUN_TEST(test_frame_606_analog_and_all_output_flag_groups);
    RUN_TEST(test_frame_606_can_switches_1_through_8);
    RUN_TEST(test_frame_607_targets_pwm_dsg_and_fuel_used);
    RUN_TEST(test_decoder_rejects_bad_id_and_dlc_without_updates);
    RUN_TEST(test_base_id_can_be_relocated);
    return UNITY_END();
}
