#include <unity.h>

// Production changes caught: incorrect conversion constants, conversions being
// applied to the wrong parameter family, or a non-reversible warning threshold.

#include "ui/unit_presenter.h"

void test_temperature_presents_fahrenheit_and_round_trips_to_celsius() {
    UnitSettings settings;
    settings.temperature = TemperatureUnit::Fahrenheit;

    const PresentedValue shown =
        UnitPresenter::present(ParameterId::Clt, 100.0f, settings);

    TEST_ASSERT_FLOAT_WITHIN(0.001f, 212.0f, shown.value);
    TEST_ASSERT_EQUAL_STRING("°F", shown.unit);
    TEST_ASSERT_FLOAT_WITHIN(
        0.001f, 100.0f,
        UnitPresenter::toNative(ParameterId::Clt, shown.value, settings));
}

void test_pressure_presents_psi_and_round_trips_to_bar() {
    UnitSettings settings;
    settings.pressure = PressureUnit::Psi;

    const PresentedValue shown =
        UnitPresenter::present(ParameterId::OilPressure, 2.0f, settings);

    TEST_ASSERT_FLOAT_WITHIN(0.001f, 29.0075476f, shown.value);
    TEST_ASSERT_EQUAL_STRING("psi", shown.unit);
    TEST_ASSERT_FLOAT_WITHIN(
        0.001f, 2.0f,
        UnitPresenter::toNative(ParameterId::OilPressure, shown.value, settings));
}

void test_pressure_presents_kpa_and_round_trips_to_bar() {
    UnitSettings settings;
    settings.pressure = PressureUnit::Kpa;

    const PresentedValue shown =
        UnitPresenter::present(ParameterId::Map, 1.25f, settings);

    TEST_ASSERT_FLOAT_WITHIN(0.001f, 125.0f, shown.value);
    TEST_ASSERT_EQUAL_STRING("kPa", shown.unit);
    TEST_ASSERT_FLOAT_WITHIN(
        0.001f, 1.25f,
        UnitPresenter::toNative(ParameterId::Map, shown.value, settings));
}

void test_speed_presents_mph_and_round_trips_to_kph() {
    UnitSettings settings;
    settings.speed = SpeedUnit::Mph;

    const PresentedValue shown =
        UnitPresenter::present(ParameterId::Speed, 100.0f, settings);

    TEST_ASSERT_FLOAT_WITHIN(0.001f, 62.1371192f, shown.value);
    TEST_ASSERT_EQUAL_STRING("mph", shown.unit);
    TEST_ASSERT_FLOAT_WITHIN(
        0.001f, 100.0f,
        UnitPresenter::toNative(ParameterId::Speed, shown.value, settings));
}

void test_lambda_presents_afr_using_configured_stoich_and_round_trips() {
    UnitSettings settings;
    settings.mixture = MixtureUnit::Afr;
    settings.stoich_afr = 14.7f;

    const PresentedValue shown =
        UnitPresenter::present(ParameterId::Lambda, 0.85f, settings);

    TEST_ASSERT_FLOAT_WITHIN(0.001f, 12.495f, shown.value);
    TEST_ASSERT_EQUAL_STRING("AFR", shown.unit);
    TEST_ASSERT_FLOAT_WITHIN(
        0.001f, 0.85f,
        UnitPresenter::toNative(ParameterId::Lambda, shown.value, settings));
}

void test_unselected_unit_family_keeps_native_value_and_unit() {
    UnitSettings settings;
    settings.pressure = PressureUnit::Psi;
    settings.temperature = TemperatureUnit::Fahrenheit;
    settings.speed = SpeedUnit::Mph;
    settings.mixture = MixtureUnit::Afr;

    const PresentedValue shown =
        UnitPresenter::present(ParameterId::BatteryVoltage, 13.8f, settings);

    TEST_ASSERT_FLOAT_WITHIN(0.001f, 13.8f, shown.value);
    TEST_ASSERT_EQUAL_STRING("V", shown.unit);
}

void test_extended_native_units_have_unambiguous_labels() {
    const UnitSettings settings;

    TEST_ASSERT_EQUAL_STRING(
        "deg", UnitPresenter::present(ParameterId::IgnitionTiming, 12.5f,
                                       settings).unit);
    TEST_ASSERT_EQUAL_STRING(
        "ms", UnitPresenter::present(ParameterId::InjectorPulseWidth, 3.2f,
                                      settings).unit);
    TEST_ASSERT_EQUAL_STRING(
        "g/s", UnitPresenter::present(ParameterId::MassAirFlow, 125.0f,
                                       settings).unit);
}

void test_extended_parameters_reuse_their_unit_family_settings() {
    UnitSettings settings;
    settings.temperature = TemperatureUnit::Fahrenheit;
    settings.pressure = PressureUnit::Kpa;
    settings.speed = SpeedUnit::Mph;
    settings.mixture = MixtureUnit::Afr;
    settings.stoich_afr = 14.7f;

    TEST_ASSERT_FLOAT_WITHIN(
        0.001f, 1832.0f,
        UnitPresenter::present(ParameterId::Egt1, 1000.0f, settings).value);
    TEST_ASSERT_FLOAT_WITHIN(
        0.001f, 101.3f,
        UnitPresenter::present(ParameterId::BarometricPressure, 1.013f,
                               settings).value);
    TEST_ASSERT_FLOAT_WITHIN(
        0.001f, 62.1371192f,
        UnitPresenter::present(ParameterId::WheelSpeedRf, 100.0f,
                               settings).value);
    TEST_ASSERT_FLOAT_WITHIN(
        0.001f, 14.7f,
        UnitPresenter::present(ParameterId::Lambda2, 1.0f, settings).value);
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_temperature_presents_fahrenheit_and_round_trips_to_celsius);
    RUN_TEST(test_pressure_presents_psi_and_round_trips_to_bar);
    RUN_TEST(test_pressure_presents_kpa_and_round_trips_to_bar);
    RUN_TEST(test_speed_presents_mph_and_round_trips_to_kph);
    RUN_TEST(test_lambda_presents_afr_using_configured_stoich_and_round_trips);
    RUN_TEST(test_unselected_unit_family_keeps_native_value_and_unit);
    RUN_TEST(test_extended_native_units_have_unambiguous_labels);
    RUN_TEST(test_extended_parameters_reuse_their_unit_family_settings);
    return UNITY_END();
}
