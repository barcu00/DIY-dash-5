#include <cstring>

#include <unity.h>

// Production changes caught: distance integrated across data gaps or from
// simulated DEMO data, wrong correction sign, trips not reset, persistence
// cadence regressions and corrupted records being accepted.

#include "odometer/distance_source.h"
#include "odometer/odometer_model.h"
#include "telemetry/composite_telemetry_view.h"

namespace {
void drive(OdometerModel& model, float kph, uint32_t from_ms, uint32_t to_ms,
           uint32_t step_ms = 20U) {
    for (uint32_t t = from_ms; t <= to_ms; t += step_ms) {
        model.integrate(SpeedSample{kph, true}, t);
    }
}
}  // namespace

void test_constant_speed_accumulates_expected_distance() {
    OdometerModel model;
    drive(model, 72.0f, 0U, 10000U);  // 20 m/s for 10 s
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.200f, static_cast<float>(model.odometerKm()));
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.200f, static_cast<float>(model.tripKm(TripId::A)));
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.200f, static_cast<float>(model.tripKm(TripId::B)));
    TEST_ASSERT_TRUE(model.get(ParameterId::Odometer).valid);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.200f, model.get(ParameterId::TripA).value);
}

void test_gaps_and_invalid_samples_are_not_bridged() {
    OdometerModel model;
    model.integrate(SpeedSample{100.0f, true}, 0U);
    model.integrate(SpeedSample{100.0f, true}, 1000U);  // 1 s gap > 250 ms
    TEST_ASSERT_EQUAL_FLOAT(0.0f, static_cast<float>(model.odometerKm()));
    model.integrate(SpeedSample{100.0f, false}, 1020U);
    model.integrate(SpeedSample{100.0f, true}, 1040U);
    TEST_ASSERT_EQUAL_FLOAT(0.0f, static_cast<float>(model.odometerKm()));
}

void test_crawl_below_one_kph_is_standstill() {
    OdometerModel model;
    drive(model, 0.8f, 0U, 60000U);
    TEST_ASSERT_EQUAL_FLOAT(0.0f, static_cast<float>(model.odometerKm()));
    TEST_ASSERT_EQUAL_UINT32(0U, static_cast<uint32_t>(model.tripMovingMs(TripId::A)));
}

void test_correction_scales_distance() {
    OdometerModel model;
    OdometerSettings settings;
    settings.correction_permille = 20;  // +2.0 %
    TEST_ASSERT_TRUE(model.setSettings(settings));
    drive(model, 36.0f, 0U, 100000U);  // 10 m/s for 100 s = 1 km
    TEST_ASSERT_FLOAT_WITHIN(0.0005f, 1.020f, static_cast<float>(model.odometerKm()));
    settings.correction_permille = 101;
    TEST_ASSERT_FALSE(model.setSettings(settings));
}

void test_set_and_trip_reset_request_immediate_save() {
    OdometerModel model;
    drive(model, 36.0f, 0U, 2000U);
    TEST_ASSERT_TRUE(model.setOdometerKm(184532.4, 2000U));
    TEST_ASSERT_TRUE(model.persistDue(2000U));
    model.markPersisted(2000U);
    TEST_ASSERT_FALSE(model.persistDue(2001U));
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 184532.4f, static_cast<float>(model.odometerKm()));
    TEST_ASSERT_TRUE(model.tripKm(TripId::A) > 0.0);
    model.resetTrip(TripId::A, 2002U);
    TEST_ASSERT_EQUAL_FLOAT(0.0f, static_cast<float>(model.tripKm(TripId::A)));
    TEST_ASSERT_TRUE(model.tripKm(TripId::B) > 0.0);
    TEST_ASSERT_TRUE(model.persistDue(2002U));
    TEST_ASSERT_FALSE(model.setOdometerKm(-1.0, 2003U));
}

void test_saves_every_kilometre_while_moving_and_at_stops() {
    OdometerModel model;
    drive(model, 36.0f, 0U, 50000U);  // 500 m, still moving
    TEST_ASSERT_FALSE(model.persistDue(50000U));
    drive(model, 36.0f, 50020U, 100100U);  // > 1 km
    TEST_ASSERT_TRUE(model.persistDue(100100U));
    model.markPersisted(100100U);

    drive(model, 36.0f, 100120U, 110000U);  // 100 m more, then stop
    drive(model, 0.0f, 110020U, 111000U);
    TEST_ASSERT_FALSE(model.persistDue(111000U));
    drive(model, 0.0f, 111020U, 112100U);
    TEST_ASSERT_TRUE(model.persistDue(112100U));
}

void test_failed_save_backs_off_before_retry() {
    OdometerModel model;
    model.resetTrip(TripId::B, 0U);
    TEST_ASSERT_TRUE(model.persistDue(0U));
    model.persistFailed(0U);
    TEST_ASSERT_TRUE(model.storageError());
    TEST_ASSERT_FALSE(model.persistDue(1000U));
    TEST_ASSERT_TRUE(model.persistDue(OdometerModel::kRetryAfterFailureMs));
}

void test_record_round_trip_and_crc_rejects_corruption() {
    OdometerModel model;
    OdometerSettings settings;
    settings.source = DistanceSource::Wheels;
    settings.gps_fallback = false;
    settings.correction_permille = -15;
    model.setSettings(settings);
    model.setOdometerKm(1234.5, 0U);
    drive(model, 36.0f, 0U, 10000U);

    OdometerRecord record = model.record();
    TEST_ASSERT_TRUE(odometerRecordValid(record));
    OdometerModel restored;
    restored.restore(record, 0U);
    TEST_ASSERT_FLOAT_WITHIN(0.0002f, static_cast<float>(model.odometerKm()),
                             static_cast<float>(restored.odometerKm()));
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(DistanceSource::Wheels),
                            static_cast<uint8_t>(restored.settings().source));
    TEST_ASSERT_FALSE(restored.settings().gps_fallback);
    TEST_ASSERT_EQUAL_INT16(-15, restored.settings().correction_permille);
    TEST_ASSERT_EQUAL_UINT32(static_cast<uint32_t>(model.tripMovingMs(TripId::A)),
                             static_cast<uint32_t>(restored.tripMovingMs(TripId::A)));

    record.odometer_dm += 1U;
    TEST_ASSERT_FALSE(odometerRecordValid(record));
    record = model.record();
    record.version = 2U;
    TEST_ASSERT_FALSE(odometerRecordValid(record));
}

void test_late_load_merges_distance_driven_since_boot() {
    OdometerModel stored;
    stored.setOdometerKm(1000.0, 0U);
    const OdometerRecord record = stored.record();

    OdometerModel model;
    drive(model, 36.0f, 0U, 10000U);  // 100 m before storage came back
    model.mergeStored(record, 10000U);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 1000.1f, static_cast<float>(model.odometerKm()));
    TEST_ASSERT_TRUE(model.persistDue(10000U));
}

void test_distance_source_selection_and_demo_exclusion() {
    VehicleState engine;
    engine.reset(DataSource::Can);
    RaceChronoTelemetry racechrono;
    OdometerModel model;
    const CompositeTelemetryView view(engine, racechrono, &model);
    OdometerSettings settings;

    TEST_ASSERT_FALSE(selectDistanceSpeed(settings, view, false).valid);
    engine.set(ParameterId::Speed, 88.0f, 0U);
    SpeedSample sample = selectDistanceSpeed(settings, view, false);
    TEST_ASSERT_TRUE(sample.valid);
    TEST_ASSERT_EQUAL_FLOAT(88.0f, sample.kph);
    TEST_ASSERT_FALSE(selectDistanceSpeed(settings, view, true).valid);

    settings.source = DistanceSource::Wheels;
    engine.set(ParameterId::WheelSpeedLf, 80.0f, 0U);
    engine.set(ParameterId::WheelSpeedRf, 90.0f, 0U);
    sample = selectDistanceSpeed(settings, view, false);
    TEST_ASSERT_TRUE(sample.valid);
    TEST_ASSERT_EQUAL_FLOAT(85.0f, sample.kph);

    TEST_ASSERT_TRUE(view.get(ParameterId::Odometer).valid);
    TEST_ASSERT_FALSE(engine.get(ParameterId::Odometer).valid);
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_constant_speed_accumulates_expected_distance);
    RUN_TEST(test_gaps_and_invalid_samples_are_not_bridged);
    RUN_TEST(test_crawl_below_one_kph_is_standstill);
    RUN_TEST(test_correction_scales_distance);
    RUN_TEST(test_set_and_trip_reset_request_immediate_save);
    RUN_TEST(test_saves_every_kilometre_while_moving_and_at_stops);
    RUN_TEST(test_failed_save_backs_off_before_retry);
    RUN_TEST(test_record_round_trip_and_crc_rejects_corruption);
    RUN_TEST(test_late_load_merges_distance_driven_since_boot);
    RUN_TEST(test_distance_source_selection_and_demo_exclusion);
    return UNITY_END();
}
