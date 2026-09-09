#include <initializer_list>

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

void test_ecumaster_extended_engine_channels_decode_to_native_units() {
    VehicleState combustion = decode(
        "ecumaster_emu_black",
        CanFrame{0x603U, 8U,
                 {20U, 0U, 128U, 0U, 0x84U, 0x03U, 0xB6U, 0x03U},
                 false, false});
    assertValue(combustion, ParameterId::IgnitionTiming, 10.0f);
    assertValue(combustion, ParameterId::Egt1, 900.0f);
    assertValue(combustion, ParameterId::Egt2, 950.0f);
    TEST_ASSERT_FALSE(combustion.get(ParameterId::Lambda2).valid);

    VehicleState status = decode(
        "ecumaster_emu_black",
        CanFrame{0x604U, 8U, {3U, 0U, 0xF4U, 0x01U, 0U, 0U, 0U, 85U},
                 false, false});
    assertValue(status, ParameterId::EthanolContent, 85.0f);

    VehicleState target = decode(
        "ecumaster_emu_black",
        CanFrame{0x607U, 8U, {150U, 0U, 0U, 0U, 0U, 0U, 0U, 0U},
                 false, false});
    assertValue(target, ParameterId::BoostTarget, 1.5f);
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

void test_rusefi_extended_channels_decode_to_native_units() {
    VehicleState fast = decode(
        "rusefi_verbose",
        CanFrame{0x201U, 8U,
                 {0x94U, 0x11U, 0xE8U, 0x03U, 100U, 0U, 123U, 85U},
                 false, false});
    assertValue(fast, ParameterId::IgnitionTiming, 20.0f);
    assertValue(fast, ParameterId::InjectorDuty, 50.0f);
    assertValue(fast, ParameterId::EthanolContent, 85.0f);

    VehicleState fueling = decode(
        "rusefi_verbose",
        CanFrame{0x205U, 8U,
                 {0U, 0U, 0x10U, 0x0EU, 0x84U, 0x03U, 0U, 0U},
                 false, false});
    assertValue(fueling, ParameterId::MassAirFlow, 10.0f, 0.001f);
    assertValue(fueling, ParameterId::InjectorPulseWidth, 3.0f, 0.001f);

    VehicleState lambda = decode(
        "rusefi_verbose",
        CanFrame{0x207U, 8U,
                 {0x10U, 0x27U, 0x28U, 0x23U, 0xB8U, 0x0BU, 0U, 0U},
                 false, false});
    assertValue(lambda, ParameterId::Lambda2, 0.9f);

    VehicleState egt = decode(
        "rusefi_verbose",
        CanFrame{0x209U, 8U, {160U, 161U, 162U, 163U, 164U, 165U, 166U, 167U},
                 false, false});
    assertValue(egt, ParameterId::Egt1, 800.0f);
    assertValue(egt, ParameterId::Egt8, 835.0f);
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

void test_maxxecu_documented_extended_channels_decode_to_native_units() {
    VehicleState fast2 = decode(
        "maxxecu_default_1_3",
        CanFrame{0x521U, 8U,
                 {0xE8U, 0x03U, 0x84U, 0x03U, 200U, 0U, 0U, 0U},
                 false, false});
    assertValue(fast2, ParameterId::Lambda2, 0.9f);
    assertValue(fast2, ParameterId::IgnitionTiming, 20.0f);

    VehicleState injection = decode(
        "maxxecu_default_1_3",
        CanFrame{0x522U, 8U,
                 {0x2CU, 0x01U, 0xF4U, 0x01U, 0U, 0U, 0xE8U, 0x03U},
                 false, false});
    assertValue(injection, ParameterId::InjectorPulseWidth, 3.0f);
    assertValue(injection, ParameterId::InjectorDuty, 50.0f);

    VehicleState sensors = decode(
        "maxxecu_default_1_3",
        CanFrame{0x530U, 8U,
                 {0x64U, 0x05U, 0xF4U, 0x03U, 0x2CU, 0x01U, 0x84U, 0x03U},
                 false, false});
    assertValue(sensors, ParameterId::BarometricPressure, 1.012f);

    VehicleState ethanol_egt = decode(
        "maxxecu_default_1_3",
        CanFrame{0x531U, 8U,
                 {0U, 0U, 0x52U, 0x03U, 0U, 0U, 0x84U, 0x03U},
                 false, false});
    assertValue(ethanol_egt, ParameterId::EthanolContent, 85.0f);
    assertValue(ethanol_egt, ParameterId::Egt1, 900.0f);

    VehicleState egt2_5 = decode(
        "maxxecu_default_1_3",
        CanFrame{0x532U, 8U,
                 {0x8EU, 0x03U, 0x98U, 0x03U, 0xA2U, 0x03U, 0xACU, 0x03U},
                 false, false});
    assertValue(egt2_5, ParameterId::Egt2, 910.0f);
    assertValue(egt2_5, ParameterId::Egt5, 940.0f);

    VehicleState egt6_8 = decode(
        "maxxecu_default_1_3",
        CanFrame{0x533U, 8U,
                 {0xB6U, 0x03U, 0xC0U, 0x03U, 0xCAU, 0x03U, 0U, 0U},
                 false, false});
    assertValue(egt6_8, ParameterId::Egt6, 950.0f);
    assertValue(egt6_8, ParameterId::Egt8, 970.0f);

    VehicleState pressures = decode(
        "maxxecu_default_1_3",
        CanFrame{0x537U, 8U,
                 {0xB8U, 0x0BU, 0U, 0U, 0x4CU, 0x04U, 0xDCU, 0x05U},
                 false, false});
    assertValue(pressures, ParameterId::CoolantPressure, 1.1f);
    assertValue(pressures, ParameterId::BoostTarget, 1.5f);
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

void test_haltech_family_extended_broadcast_channels_decode() {
    VehicleState haltech_injection = decode(
        "haltech_broadcast_2_0",
        CanFrame{0x362U, 8U,
                 {0x01U, 0xF4U, 0U, 0U, 0x00U, 0xC8U, 0U, 0U},
                 false, false});
    assertValue(haltech_injection, ParameterId::InjectorDuty, 50.0f);
    assertValue(haltech_injection, ParameterId::IgnitionTiming, 20.0f);

    VehicleState speeduino_injection = decode(
        "speeduino_haltech",
        CanFrame{0x362U, 8U,
                 {0x00U, 0x32U, 0U, 0U, 0x00U, 0xC8U, 0U, 0U},
                 false, false});
    assertValue(speeduino_injection, ParameterId::InjectorDuty, 50.0f);
    assertValue(speeduino_injection, ParameterId::IgnitionTiming, 20.0f);

    for (const char* profile : {"haltech_broadcast_2_0", "speeduino_haltech"}) {
        VehicleState pulse = decode(
            profile,
            CanFrame{0x364U, 8U,
                     {0x0BU, 0xB8U, 0U, 0U, 0U, 0U, 0U, 0U},
                     false, false});
        assertValue(pulse, ParameterId::InjectorPulseWidth, 3.0f);

        VehicleState mixture = decode(
            profile,
            CanFrame{0x368U, 8U,
                     {0x03U, 0xE8U, 0x03U, 0x84U, 0U, 0U, 0U, 0U},
                     false, false});
        assertValue(mixture, ParameterId::Lambda2, 0.9f);
    }
}

void test_haltech_coolant_pressure_uses_atmospheric_offset() {
    VehicleState state = decode(
        "haltech_broadcast_2_0",
        CanFrame{0x360U, 8U,
                 {0x0FU, 0xA0U, 0x05U, 0xDCU,
                  0x01U, 0xF4U, 0x09U, 0xD8U},
                 false, false});
    assertValue(state, ParameterId::CoolantPressure, 1.507f, 0.001f);
}

void test_haltech_reverse_gear_does_not_discard_vehicle_speed() {
    VehicleState state = decode(
        "haltech_broadcast_2_0",
        CanFrame{0x370U, 8U,
                 {0x03U, 0xE8U, 0U, 0xFFU, 0U, 0U, 0U, 0U},
                 false, false});
    assertValue(state, ParameterId::Speed, 100.0f);
    assertValue(state, ParameterId::Gear, -1.0f);
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

void test_link_experimental_profile_exposes_documented_extra_channels() {
    VehicleState airflow = decode(
        "link_generic_dash_experimental",
        CanFrame{0x3E8U, 8U, {3U, 0U, 0U, 0U, 0U, 0U, 0xE8U, 0x03U},
                 false, false});
    assertValue(airflow, ParameterId::MassAirFlow, 100.0f);

    VehicleState rear_wheels = decode(
        "link_generic_dash_experimental",
        CanFrame{0x3E8U, 8U,
                 {9U, 0U, 0xE8U, 0x03U, 0xF2U, 0x03U, 0xFCU, 0x03U},
                 false, false});
    assertValue(rear_wheels, ParameterId::WheelSpeedLr, 100.0f);
    assertValue(rear_wheels, ParameterId::WheelSpeedRf, 101.0f);
    assertValue(rear_wheels, ParameterId::WheelSpeedRr, 102.0f);

    VehicleState driver = decode(
        "link_generic_dash_experimental",
        CanFrame{0x3E8U, 8U,
                 {13U, 0U, 0xF4U, 0x01U, 0x52U, 0x03U, 0U, 0U},
                 false, false});
    assertValue(driver, ParameterId::AcceleratorPosition, 50.0f);
    assertValue(driver, ParameterId::EthanolContent, 85.0f);
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

void test_bmw_ms43_stock_decodes_documented_numeric_signals() {
    VehicleState fast = decode(
        "bmw_ms43_stock",
        CanFrame{0x316U, 8U,
                 {0x00U, 0U, 0x00U, 0x7DU, 0U, 0U, 0U, 0U},
                 false, false});
    assertValue(fast, ParameterId::Rpm, 5000.0f);

    VehicleState engine = decode(
        "bmw_ms43_stock",
        CanFrame{0x329U, 8U,
                 {0U, 184U, 207U, 0U, 0U, 128U, 0U, 0U},
                 false, false});
    assertValue(engine, ParameterId::Clt, 90.0f);
    assertValue(engine, ParameterId::BarometricPressure, 1.012f, 0.001f);
    assertValue(engine, ParameterId::AcceleratorPosition, 50.0f);

    VehicleState oil = decode(
        "bmw_ms43_stock",
        CanFrame{0x545U, 8U, {0U, 0U, 0U, 0U, 138U, 0U, 0U, 0U},
                 false, false});
    assertValue(oil, ParameterId::OilTemperature, 90.0f);
}

void test_bmw_ms43_stock_decodes_documented_engine_flags() {
    VehicleState fast = decode(
        "bmw_ms43_stock",
        CanFrame{0x316U, 8U,
                 {0x83U, 0U, 0x00U, 0x7DU, 0U, 0U, 0U, 0U},
                 false, false});
    assertValue(fast, ParameterId::IgnitionOn, 1.0f);
    assertValue(fast, ParameterId::CrankSensorError, 1.0f);
    assertValue(fast, ParameterId::MassAirFlowError, 1.0f);

    VehicleState engine = decode(
        "bmw_ms43_stock",
        CanFrame{0x329U, 8U,
                 {0U, 184U, 207U, 0x0BU, 0U, 128U, 0x07U, 0U},
                 false, false});
    assertValue(engine, ParameterId::ClutchPressed, 1.0f);
    assertValue(engine, ParameterId::IdleActive, 1.0f);
    assertValue(engine, ParameterId::EngineRunning, 1.0f);
    assertValue(engine, ParameterId::BrakePressed, 1.0f);
    assertValue(engine, ParameterId::BrakeSystemFault, 1.0f);
    assertValue(engine, ParameterId::KickdownActive, 1.0f);

    VehicleState warnings = decode(
        "bmw_ms43_stock",
        CanFrame{0x545U, 8U,
                 {0x12U, 0U, 0U, 0x8FU, 138U, 0U, 0U, 0x80U},
                 false, false});
    assertValue(warnings, ParameterId::CheckEngine, 1.0f);
    assertValue(warnings, ParameterId::EmlWarning, 1.0f);
    assertValue(warnings, ParameterId::OilConsumptionWarning, 1.0f);
    assertValue(warnings, ParameterId::OilLossWarning, 1.0f);
    assertValue(warnings, ParameterId::OilSensorFault, 1.0f);
    assertValue(warnings, ParameterId::CoolantOverheat, 1.0f);
    assertValue(warnings, ParameterId::UpshiftRequest, 1.0f);
    assertValue(warnings, ParameterId::LowOilPressure, 1.0f);
}

void test_bmw_ms43_stock_flag_bits_clear_to_off() {
    VehicleState state = decode(
        "bmw_ms43_stock",
        CanFrame{0x545U, 8U, {0U, 0U, 0U, 0U, 138U, 0U, 0U, 0U},
                 false, false});
    assertValue(state, ParameterId::CheckEngine, 0.0f);
    assertValue(state, ParameterId::EmlWarning, 0.0f);
    assertValue(state, ParameterId::LowOilPressure, 0.0f);
}

void test_ecumaster_decodes_all_approved_status_flags() {
    VehicleState status = decode(
        "ecumaster_emu_black",
        CanFrame{0x604U, 8U,
                 {3U, 0U, 0xF4U, 0x01U, 0xFFU, 0x07U, 0x6FU, 85U},
                 false, false});
    for (const ParameterId flag : {
             ParameterId::CoolantSensorError,
             ParameterId::IntakeAirSensorError,
             ParameterId::MapSensorError,
             ParameterId::WidebandSensorError,
             ParameterId::Egt1SensorError,
             ParameterId::Egt2SensorError,
             ParameterId::EgtHighAlarm,
             ParameterId::KnockDetected,
             ParameterId::FlexFuelSensorError,
             ParameterId::DbwError,
             ParameterId::FuelPressureError,
             ParameterId::GearCutActive,
             ParameterId::AntiLagActive,
             ParameterId::LaunchControlActive,
             ParameterId::IdleActive,
             ParameterId::TractionControlActive,
             ParameterId::PitLimiterActive,
         }) {
        assertValue(status, flag, 1.0f);
    }

    VehicleState outputs = decode(
        "ecumaster_emu_black",
        CanFrame{0x606U, 8U, {0U, 0U, 0U, 0U, 0U, 0U, 0U, 0x3FU},
                 false, false});
    for (const ParameterId flag : {
             ParameterId::FuelPumpActive,
             ParameterId::CoolantFanActive,
             ParameterId::AcClutchActive,
             ParameterId::AcFanActive,
             ParameterId::NitrousActive,
             ParameterId::StarterRequestActive,
         }) {
        assertValue(outputs, flag, 1.0f);
    }
}

void test_rusefi_decodes_all_approved_status_flags() {
    VehicleState status = decode(
        "rusefi_verbose",
        CanFrame{0x200U, 8U, {0U, 0U, 0U, 0U, 0xFFU, 3U, 0U, 0U},
                 false, false});
    for (const ParameterId flag : {
             ParameterId::RevLimiterActive,
             ParameterId::MainRelayActive,
             ParameterId::FuelPumpActive,
             ParameterId::CheckEngine,
             ParameterId::O2HeaterActive,
             ParameterId::LambdaProtectionActive,
             ParameterId::CoolantFanActive,
             ParameterId::CoolantFan2Active,
         }) {
        assertValue(status, flag, 1.0f);
    }

    VehicleState brake = decode(
        "rusefi_verbose",
        CanFrame{0x20BU, 8U, {0x01U, 0U, 0U, 0U, 0U, 0U, 0U, 0U},
                 false, false});
    assertValue(brake, ParameterId::BrakePressed, 1.0f);
}

void test_maxxecu_decodes_all_approved_status_flags() {
    VehicleState status = decode(
        "maxxecu_default_1_3",
        CanFrame{0x526U, 8U, {0xFFU, 0x7FU, 0U, 0U, 0U, 0U, 0U, 0U},
                 false, false});
    for (const ParameterId flag : {
             ParameterId::ShiftCutActive,
             ParameterId::RevLimiterActive,
             ParameterId::AntiLagActive,
             ParameterId::LaunchControlActive,
             ParameterId::TractionPowerLimiterActive,
             ParameterId::ThrottleBlipActive,
             ParameterId::AcIdleUpActive,
             ParameterId::KnockDetected,
             ParameterId::BrakePressed,
             ParameterId::ClutchPressed,
             ParameterId::SpeedLimiterActive,
             ParameterId::GpLimiterActive,
             ParameterId::UserCutActive,
             ParameterId::EcuLoggingActive,
             ParameterId::NitrousActive,
         }) {
        assertValue(status, flag, 1.0f);
    }
}

void test_link_decodes_limit_bits_and_exact_active_feature_states() {
    VehicleState limits = decode(
        "link_generic_dash_experimental",
        CanFrame{0x3E8U, 8U, {12U, 0U, 0U, 0U, 0U, 0U, 0xFFU, 0xFFU},
                 false, false});
    for (const ParameterId flag : {
             ParameterId::RevLimiterActive,
             ParameterId::MapLimiterActive,
             ParameterId::SpeedLimiterActive,
             ParameterId::MaxIgnitionLimiterActive,
             ParameterId::AntiLagIgnitionCutActive,
             ParameterId::HighVoltageLimitActive,
             ParameterId::OverrunActive,
             ParameterId::TractionPowerLimiterActive,
             ParameterId::LowVoltageLimitActive,
             ParameterId::LaunchRpmLimitActive,
             ParameterId::WakeupActive,
             ParameterId::GpRpmLimit1Active,
             ParameterId::ClosedLoopStepperLimitActive,
             ParameterId::GpRpmLimit2Active,
             ParameterId::EThrottleLimitActive,
             ParameterId::CyclicIdleActive,
         }) {
        assertValue(limits, flag, 1.0f);
    }

    VehicleState active = decode(
        "link_generic_dash_experimental",
        CanFrame{0x3E8U, 8U, {13U, 0U, 0U, 0U, 0U, 0U, 0x2DU, 0x20U},
                 false, false});
    assertValue(active, ParameterId::AntiLagActive, 1.0f);
    assertValue(active, ParameterId::LaunchControlActive, 1.0f);
    assertValue(active, ParameterId::TractionControlActive, 1.0f);
    assertValue(active, ParameterId::CruiseControlActive, 1.0f);

    VehicleState inactive = decode(
        "link_generic_dash_experimental",
        CanFrame{0x3E8U, 8U, {13U, 0U, 0U, 0U, 0U, 0U, 0x52U, 0x10U},
                 false, false});
    assertValue(inactive, ParameterId::AntiLagActive, 0.0f);
    assertValue(inactive, ParameterId::LaunchControlActive, 0.0f);
    assertValue(inactive, ParameterId::TractionControlActive, 0.0f);
    assertValue(inactive, ParameterId::CruiseControlActive, 0.0f);
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
    RUN_TEST(test_ecumaster_extended_engine_channels_decode_to_native_units);
    RUN_TEST(test_rusefi_core_frames_decode_to_native_units);
    RUN_TEST(test_rusefi_extended_channels_decode_to_native_units);
    RUN_TEST(test_maxxecu_core_frames_decode_to_native_units);
    RUN_TEST(test_maxxecu_documented_extended_channels_decode_to_native_units);
    RUN_TEST(test_haltech_and_speeduino_compatible_frames_decode_big_endian);
    RUN_TEST(test_haltech_family_extended_broadcast_channels_decode);
    RUN_TEST(test_haltech_coolant_pressure_uses_atmospheric_offset);
    RUN_TEST(test_haltech_reverse_gear_does_not_discard_vehicle_speed);
    RUN_TEST(test_link_indexed_frames_require_the_discriminator);
    RUN_TEST(test_link_experimental_profile_exposes_documented_extra_channels);
    RUN_TEST(test_psa_profile_decodes_only_corroborated_engine_sensors);
    RUN_TEST(test_bmw_ms43_stock_decodes_documented_numeric_signals);
    RUN_TEST(test_bmw_ms43_stock_decodes_documented_engine_flags);
    RUN_TEST(test_bmw_ms43_stock_flag_bits_clear_to_off);
    RUN_TEST(test_ecumaster_decodes_all_approved_status_flags);
    RUN_TEST(test_rusefi_decodes_all_approved_status_flags);
    RUN_TEST(test_maxxecu_decodes_all_approved_status_flags);
    RUN_TEST(test_link_decodes_limit_bits_and_exact_active_feature_states);
    RUN_TEST(test_wrong_dlc_and_format_are_rejected_without_partial_update);
    RUN_TEST(test_out_of_range_signal_rejects_the_whole_matching_frame);
    return UNITY_END();
}
