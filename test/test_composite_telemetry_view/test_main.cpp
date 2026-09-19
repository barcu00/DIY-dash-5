#include <unity.h>

#include "telemetry/composite_telemetry_view.h"

void test_engine_and_racechrono_values_coexist_without_overwriting() {
    VehicleState engine;
    engine.reset(DataSource::Can);
    engine.set(ParameterId::Rpm, 6840.0f, 100U);
    engine.set(ParameterId::RcDeltaLapTime, 99.0f, 100U);
    RaceChronoTelemetry racechrono;
    racechrono.acceptRaw(10U, -240, 100U);
    CompositeTelemetryView view(engine, racechrono);

    TEST_ASSERT_FLOAT_WITHIN(
        0.001f, 6840.0f, view.get(ParameterId::Rpm).value);
    TEST_ASSERT_FLOAT_WITHIN(
        0.001f, -0.240f,
        view.get(ParameterId::RcDeltaLapTime).value);
    TEST_ASSERT_EQUAL_PTR(&engine, &view.engineState());
    TEST_ASSERT_EQUAL_PTR(&racechrono, &view.raceChronoState());
}

void test_supplement_availability_is_independent_of_engine_source() {
    VehicleState engine;
    engine.reset(DataSource::Demo);
    RaceChronoTelemetry racechrono;
    CompositeTelemetryView view(engine, racechrono);

    TEST_ASSERT_FALSE(view.availableFromSupplement(ParameterId::RcGpsSpeed));
    racechrono.acceptRaw(17U, 1234, 100U);
    TEST_ASSERT_TRUE(view.availableFromSupplement(ParameterId::RcGpsSpeed));
    TEST_ASSERT_FALSE(view.availableFromSupplement(ParameterId::Speed));
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_engine_and_racechrono_values_coexist_without_overwriting);
    RUN_TEST(test_supplement_availability_is_independent_of_engine_source);
    return UNITY_END();
}
