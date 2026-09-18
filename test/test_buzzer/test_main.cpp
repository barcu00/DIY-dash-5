#include <unity.h>
#include <cstdint>
#include "alarms/buzzer_model.h"
#include "alarms/tile_warning_engine.h"

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
    TEST_ASSERT_TRUE(b.update(UINT32_MAX-99,true,false));
    TEST_ASSERT_TRUE(b.update(99,true,false));
    TEST_ASSERT_FALSE(b.update(100,true,false));
    TEST_ASSERT_FALSE(b.update(549,true,false));
    TEST_ASSERT_FALSE(b.update(550,true,false));
    BuzzerModel warning; warning.begin(0); warning.update(0,true,false);
    warning.update(650,true,false);
    TEST_ASSERT_TRUE(warning.update(UINT32_MAX-49,true,true));
    TEST_ASSERT_FALSE(warning.update(100,true,true));
    TEST_ASSERT_TRUE(warning.update(550,true,true));
}
void test_real_warning_delay_acknowledge_and_rebreach() {
    BuzzerModel b; b.begin(0); b.update(200,true,false);
    auto config=AppConfig::defaults();
    auto& tile=config.dash_tiles[0]; tile.parameter=ParameterId::Clt;
    tile.warning={true,WarningDirection::Above,50.0f,1.0f,100};
    VehicleState state; state.reset(DataSource::Demo);
    TileWarningEngine warnings;
    state.set(ParameterId::Clt,60,1000); warnings.evaluate(config,state,1000);
    TEST_ASSERT_FALSE(b.update(1000,true,warnings.nextModal().has_value()));
    warnings.evaluate(config,state,1100);
    TEST_ASSERT_TRUE(b.update(1100,true,warnings.nextModal().has_value()));
    warnings.acknowledge({PageId::Dash,0});
    TEST_ASSERT_FALSE(b.update(1101,true,warnings.nextModal().has_value()));
    TEST_ASSERT_TRUE(warnings.isHighlighted({PageId::Dash,0}));
    state.set(ParameterId::Clt,40,1150); warnings.evaluate(config,state,1150);
    TEST_ASSERT_FALSE(b.update(1150,true,warnings.nextModal().has_value()));
    state.set(ParameterId::Clt,60,1200); warnings.evaluate(config,state,1200);
    TEST_ASSERT_FALSE(b.update(1200,true,warnings.nextModal().has_value()));
    warnings.evaluate(config,state,1300);
    TEST_ASSERT_TRUE(b.update(1300,true,warnings.nextModal().has_value()));
}
void test_delayed_first_update_still_produces_full_startup_chirp() {
    BuzzerModel b; b.begin(0);
    TEST_ASSERT_TRUE(b.update(1000,false,false));
    TEST_ASSERT_TRUE(b.update(1199,false,false));
    TEST_ASSERT_FALSE(b.update(1200,false,false));
    TEST_ASSERT_FALSE(b.update(1650,false,false));
}
void test_warning_at_startup_has_quiet_gap_after_test_chirp() {
    BuzzerModel b; b.begin(0);
    TEST_ASSERT_TRUE(b.update(0,true,true));
    TEST_ASSERT_TRUE(b.update(199,true,true));
    TEST_ASSERT_FALSE(b.update(200,true,true));
    TEST_ASSERT_FALSE(b.update(649,true,true));
    TEST_ASSERT_TRUE(b.update(650,true,true));
    TEST_ASSERT_FALSE(b.update(800,true,true));
}
#if __has_include("alarms/buzzer_test_pulse.h")
#include "alarms/buzzer_test_pulse.h"
void test_manual_pulse_stops_and_respects_disabled_sound() {
    BuzzerTestPulse pulse;pulse.request(100);
    TEST_ASSERT_TRUE(pulse.update(100,true));
    TEST_ASSERT_TRUE(pulse.update(299,true));
    TEST_ASSERT_FALSE(pulse.update(300,true));
    pulse.request(400);TEST_ASSERT_FALSE(pulse.update(401,false));
    TEST_ASSERT_FALSE(pulse.update(402,true));
}
#else
void test_manual_pulse_stops_and_respects_disabled_sound() {TEST_FAIL_MESSAGE("Manual warning test pulse missing");}
#endif
int main(int,char**) {
    UNITY_BEGIN();
    RUN_TEST(test_startup_chirps_once_even_when_warning_sound_disabled);
    RUN_TEST(test_warning_pulses_and_acknowledge_rearms);
    RUN_TEST(test_disable_silences_immediately_without_replaying_startup);
    RUN_TEST(test_startup_and_warning_across_millis_rollover);
    RUN_TEST(test_real_warning_delay_acknowledge_and_rebreach);
    RUN_TEST(test_delayed_first_update_still_produces_full_startup_chirp);
    RUN_TEST(test_warning_at_startup_has_quiet_gap_after_test_chirp);
    RUN_TEST(test_manual_pulse_stops_and_respects_disabled_sound);
    return UNITY_END();
}
