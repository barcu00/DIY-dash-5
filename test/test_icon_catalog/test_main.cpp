#include <unity.h>

#include "telemetry/parameter_id.h"
#include "ui/icon_catalog.h"

namespace {
void test_every_parameter_has_a_default_icon() {
    for (uint16_t raw = 0; raw < parameterIndex(ParameterId::Count); ++raw) {
        const auto id = static_cast<ParameterId>(raw);
        TEST_ASSERT_NOT_EQUAL(static_cast<int>(IconId::Invalid),
                              static_cast<int>(IconCatalog::defaultIcon(id)));
    }
}

void test_core_parameters_use_semantic_icon_families() {
    TEST_ASSERT_EQUAL(static_cast<int>(IconId::Rpm),
                      static_cast<int>(IconCatalog::defaultIcon(ParameterId::Rpm)));
    TEST_ASSERT_EQUAL(static_cast<int>(IconId::Speed),
                      static_cast<int>(IconCatalog::defaultIcon(ParameterId::VehicleSpeed)));
    TEST_ASSERT_EQUAL(static_cast<int>(IconId::Gear),
                      static_cast<int>(IconCatalog::defaultIcon(ParameterId::Gear)));
    TEST_ASSERT_EQUAL(static_cast<int>(IconId::Throttle),
                      static_cast<int>(IconCatalog::defaultIcon(ParameterId::Tps)));
    TEST_ASSERT_EQUAL(static_cast<int>(IconId::Boost),
                      static_cast<int>(IconCatalog::defaultIcon(ParameterId::Map)));
    TEST_ASSERT_EQUAL(static_cast<int>(IconId::OilPressure),
                      static_cast<int>(IconCatalog::defaultIcon(ParameterId::OilPressure)));
    TEST_ASSERT_EQUAL(static_cast<int>(IconId::FuelPressure),
                      static_cast<int>(IconCatalog::defaultIcon(ParameterId::FuelPressure)));
    TEST_ASSERT_EQUAL(static_cast<int>(IconId::Lambda),
                      static_cast<int>(IconCatalog::defaultIcon(ParameterId::Lambda)));
    TEST_ASSERT_EQUAL(static_cast<int>(IconId::Battery),
                      static_cast<int>(IconCatalog::defaultIcon(ParameterId::BatteryVoltage)));
}

void test_status_parameters_use_relevant_status_icons() {
    TEST_ASSERT_EQUAL(static_cast<int>(IconId::Launch),
                      static_cast<int>(IconCatalog::defaultIcon(ParameterId::LaunchControlActive)));
    TEST_ASSERT_EQUAL(static_cast<int>(IconId::AntiLag),
                      static_cast<int>(IconCatalog::defaultIcon(ParameterId::AntiLagActive)));
    TEST_ASSERT_EQUAL(static_cast<int>(IconId::Traction),
                      static_cast<int>(IconCatalog::defaultIcon(ParameterId::TractionControlActive)));
    TEST_ASSERT_EQUAL(static_cast<int>(IconId::Brake),
                      static_cast<int>(IconCatalog::defaultIcon(ParameterId::BrakeActive)));
    TEST_ASSERT_EQUAL(static_cast<int>(IconId::Fan),
                      static_cast<int>(IconCatalog::defaultIcon(ParameterId::FanActive)));
    TEST_ASSERT_EQUAL(static_cast<int>(IconId::Nitrous),
                      static_cast<int>(IconCatalog::defaultIcon(ParameterId::NitrousActive)));
    TEST_ASSERT_EQUAL(static_cast<int>(IconId::Warning),
                      static_cast<int>(IconCatalog::defaultIcon(ParameterId::EcuError)));
}

void test_icon_names_are_available_for_picker_ui() {
    TEST_ASSERT_EQUAL_STRING("RPM", IconCatalog::name(IconId::Rpm));
    TEST_ASSERT_EQUAL_STRING("BOOST", IconCatalog::name(IconId::Boost));
    TEST_ASSERT_EQUAL_STRING("LAMBDA / AFR", IconCatalog::name(IconId::Lambda));
    TEST_ASSERT_EQUAL_STRING("WARNING", IconCatalog::name(IconId::Warning));
    TEST_ASSERT_EQUAL_STRING("", IconCatalog::name(IconId::Invalid));
}
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_every_parameter_has_a_default_icon);
    RUN_TEST(test_core_parameters_use_semantic_icon_families);
    RUN_TEST(test_status_parameters_use_relevant_status_icons);
    RUN_TEST(test_icon_names_are_available_for_picker_ui);
    return UNITY_END();
}
